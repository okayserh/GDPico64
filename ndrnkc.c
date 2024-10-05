/**
 * ndrnkc.c
 *
 * Main program file. Calls device initialization functions
 * and sets up the main loop, which is a small monitor program
 *
 * Copyright (C) 2024  Oliver Kayser-Herold
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

// FreeRTOS
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>

#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/pio.h"
#include "hardware/interp.h"
#include "hardware/dma.h"
#include "hardware/structs/sio.h"
#include "gdp.h"
#include "ps2key.h"
#include "cas.h"
#include "key.h"
#include "xmodem_pico.h"
#include "par_bus.h"
#include "stdio_dev.h"
#include "gpio_def.h"
#include <stdio.h>
#include <string.h>


uint8_t z80_io_buf[256];

typedef struct iodev_list_s
{
  QueueHandle_t queue;
  uint32_t (*iodev_funct)(uint8_t reg, uint8_t data);
} iodev_list;

iodev_list simdev[16];

// Free RTOS Stuff
TaskHandle_t monitor_task_handle = NULL;

// Defined in sim1.c
extern void z80_emulator (void);

/****************************************************************************
 * Name: ser_putc
 *
 * Description:
 *   Initial function for the simulation of the serial card of
 *   the modular computer.
 *
 * Input Parameters:
 *   reg    - The register that is addresses
 *   data   - Data to be output to the "serial port"
 *
 * Returned Value:
 *   Returns 0 when everything worked as expected
 *
 ****************************************************************************/

uint32_t ser_putc (uint8_t reg, uint8_t data)
{
  if (reg == 0xF0)
    // Output character
    xQueueSend (stdio_dev.output_queue, &data, pdMS_TO_TICKS (1));
  change_io_reg (0xF1, 0, 0x10);

  // Indicate not busy anymore
  return (0);
}

/****************************************************************************
 * Name: fifo_irq_handler
 *
 * Description:
 *   Interrupt handler for the FIFO that handles
 *   communication between Core 1 (Parbus) and
 *   Core 0 (Device implementation)
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

void __not_in_flash_func(fifo_irq_handler)(void)
{
  // Notify fifo input task that information is pending in FIFO
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;

  // Note: If the FIFO is not emptied, the interrupt will
  // directly be activated again when the function leaves.
  // This will lead to an endless loop :-/
  uint32_t fifo_rd;
  uint8_t dev;
  while (sio_hw->fifo_st & 0x1)
    {
      fifo_rd = sio_hw->fifo_rd;
      dev = (fifo_rd >> 12) & 0xF;

      if (simdev[dev].queue != NULL)
	xQueueSendFromISR (simdev[dev].queue,
			   &fifo_rd,
			   &xHigherPriorityTaskWoken);
    }

  // If error flags persist, clear them
  if (sio_hw->fifo_st & (0x8 + 0x4))
    sio_hw->fifo_st = 0;

  portYIELD_FROM_ISR (xHigherPriorityTaskWoken);
}

/****************************************************************************
 * Name: dump_mem_area
 *
 * Description:
 *   Prints the virtual IO space to stdio.
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

void dump_mem_area ()
{
  printf ("IO\n");
  memcpy (z80_io_buf, z80_mem, 256);
  for (unsigned int i = 0; i < 256; ++i)
    {
      if ((i % 16) == 0)
	printf ("\n%02x: ", i);
      printf ("%02x ", z80_io_buf[i]);
    }

  printf ("\n\n");
}

/****************************************************************************
 * Name: reset_z80
 *
 * Description:
 *   Sends a reset signal on the Z80 bus.
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

void reset_z80 ()
{
  PIO pio = pio0;

  // Start PIOs
  pio_sm_set_enabled(pio, 0, false);
  pio_sm_set_enabled(pio, 1, false);

  gpio_put (RESET, true);
  write_io_reg (0xF1, 0);

  pio_sm_restart(pio, 0);
  pio_sm_restart(pio, 1);

  pio_sm_set_enabled(pio, 0, true);
  pio_sm_set_enabled(pio, 1, true);

  vTaskDelay (1);
  gpio_put (RESET, false);
}

/****************************************************************************
 * Name: task_monitor
 *
 * Description:
 *   Calls the device initializations in the following sequence
 *   - Pico Stdio device
 *   - PS/2 Keyboard interface
 *   - CAS group (cassette interface)
 *   - GDP (graphics card based on EF9365)
 *   - Initialize parallel bus
 *   - Start second core (used solely for handling IO requests on the
 *     parallel bus)
 *   - Reset Z80
 *   After initialization it executes the main event loop for handling the monitor 
 *   that allows controlling the IO emulation via pico stdio.
 *
 * Input Parameters:
 *   unused_arg    - Provided by task initialization, currently unused
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

void task_monitor(void* unused_arg) {
  // Initialize GPIO Pin for reset
  gpio_init (RESET);
  gpio_set_dir(RESET, GPIO_OUT);
  gpio_pull_up (RESET);  // ~RESET
  gpio_put (RESET, false);

  // Set interrupt priorities
  // DMA 0 and 1 as well as SIO_IRQ_PROC0 are set to PRIO 1
  irq_set_priority (DMA_IRQ_0, 0x40);
  irq_set_priority (DMA_IRQ_1, 0x40);
  irq_set_priority (SIO_IRQ_PROC0, 0x40);

  // Init "special" device, which encapsulates the pico stdio (via USB or UART)
  init_stdio_dev ();
  printf ("stdio dev initialized\n");

  // Initialize PS/2 keyboard driver
  init_ps2key ();
  printf ("ps2 dev initialized\n");

  // Initialize CAS interface
  init_cas ();
  printf ("CAS dev initialized\n");

  // Initialize graphics output first
  init_gdp ();
  printf ("gdp dev initialized\n");

  // Setup simdev struct
  memset (&simdev[0], 0, sizeof (iodev_list) * 16);
  //  simdev[15].iodev_funct = ser_putc;
  simdev[12].queue = cas_queue;
  simdev[7].queue = gdp_queue;
  simdev[6].queue = gdp_page_queue;

  // Clear Fifo
  uint32_t fifo_pop;
  while (multicore_fifo_pop_timeout_us(20,&fifo_pop))
	 printf ("Fifo %08x\n", fifo_pop);

  // Setup FIFO for receiving events from the core handling
  // the Z80 IO ports
  irq_set_exclusive_handler(SIO_IRQ_PROC0, fifo_irq_handler);
  irq_set_enabled(SIO_IRQ_PROC0, true);

  init_par_bus ();
  printf ("par bus initialized\n");

  // Z80 Emulator
  //multicore_launch_core1(z80_emulator);

  // The second core waits until it receives some message over the FIFO
  multicore_fifo_push_blocking((uint32_t) 0);

  reset_z80 ();

  // Now do the event loop
  int bTerm = 0;
  int renew = 1;
  while (true) {
    if (renew)
      {
	printf ("X - XModem receive\n");
	printf ("S - XModem send\n");
	printf ("D - Dump XModem buffer\n");
	printf ("T - Terminal mode\n");
	printf ("I - Dump IO buffer\n");
	printf ("C - Reset CAS bufptr\n");
	//	printf ("S - Start CAS output\n");
	printf ("R - Reset Z80\n\n");
	renew = 0;
      }

    uint8_t ch;
    if (xQueueReceive (stdio_dev.input_queue, &ch, (TickType_t) 10))
      {
	if (bTerm)
	  {
	    if (ch == 27)
	      {
		printf ("Terminal Mode Exit\n");
		bTerm = 0;
	      }
	    else
	      {
		write_io_reg (0x68, ch);
	      }
	  }
	else
	  {
	    switch (ch)
	      {
	      case 'x' :
	      case 'X' : xmodem_receive ();
		break;
	      case 's' :
	      case 'S' : xmodem_send ();
		break;
	      case 'd' :
	      case 'D' : dump_xmod_buffer ();
		break;
	      case 'i' :
	      case 'I' : dump_mem_area ();
		break;
	      case 't' :
	      case 'T' :
		printf ("Entering Terminal Mode\n");
		bTerm = 1;
		break;
	      case 'c' :
	      case 'C' :
		// Reset pointer to xmodem buffer
		// Indicate character available to CAS interface
		xmod_len = 0;
		break;
	      case 'r' :
	      case 'R' : reset_z80 ();
		break;
	      }
	    renew = 1;
	  }
      }

    // Temporary fix for PS/2 Keyboard
    if (xQueueReceive (ps2_dev.input_queue, &ch, (TickType_t) 0))
	{
	  write_io_reg (0x68, ch);
	}
  }
}

/****************************************************************************
 * Name: main
 *
 * Description:
 *   Some initializations of the pico and creation of main task.
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

int main() {
  // 160 MHz
  //set_sys_clock_khz(160000, true);

  // 148.5 MHz for 1080p @ 60 Hz
  set_sys_clock_khz(148500, true);
  // TODO: look for implications regarding the FreeRTOS Timing

  // Initialize Serial In- and Output
  stdio_init_all ();
  sleep_ms(200);

  // Task to setup devices and to monitor user inputs through stdio
  BaseType_t monitor_status = xTaskCreate(task_monitor,
					  "MONITOR_TASK",
					  1024,
					  NULL,
					  1,
					  &monitor_task_handle);

  // Start the FreeRTOS scheduler
  if (monitor_status == pdPASS) {
    vTaskStartScheduler();
  }

  // We should never get here, but just in case...
  while(true) {
    // NOP
  };
}
