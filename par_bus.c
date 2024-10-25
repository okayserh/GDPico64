/**
 * par_bus.c
 *
 * Implementation of the parallel IO bus interface to the Z80
 * and potentially other devices.
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

#include <stdio.h>
#include "pico/multicore.h"

#include "par_bus.h"
#include "par_bus.pio.h"

#include "gdp.h"
#include "ps2key.h"
#include "cas.h"
#include "key.h"

spin_lock_t io_spin_lock;

// Defined in:
// parport.S
extern void parloop (unsigned char *mem, uint32_t *regset);
extern void ioregread (void);
extern void ioregwrite (void);
extern void noaction (void);
extern void ser_sendflag (void);


// These are time critical and should be used exclusively for IO treatment
// of the Z80 Parallel bus
uint32_t __scratch_y(__STRING(z80_regset)) z80_regset[512];
uint32_t *z80_regget = &z80_regset[256];
uint8_t __scratch_y(__STRING(z80_mem)) z80_mem[256];

// Define a separate stack for core 1
uint32_t __scratch_y(__STRING(core1_stacj)) core1_stack[32];

#define __dvi_func_x(f) __scratch_x(__STRING(f)) f


/****************************************************************************
 * Name: core1_main
 *
 * Description:
 *   Main function for core1. Waits for the "start signal" from core 0
 *   and calls the assembler code that waits for input on the PIO
 *   FIFOs and processes those inputs.
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static void __dvi_func_x(core1_main)()
{
  const char *s = (const char *) multicore_fifo_pop_blocking();

  // The C code was not fast enough to get a proper bus timing.
  // Hence, a short assembler routine will watch for data requests
  // and provide them from z80_mem
  parloop (z80_mem, z80_regset);
}


/****************************************************************************
 * Name: init_par_bus
 *
 * Description:
 *   Initialization of the parallel Z80 bus interface. Initializes
 *   the PIOs for handling the parallel bus and registers the callback
 *   hooks for when IO ports are read from or written to.
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

int init_par_bus ()
{
  PIO pio = pio0;
  uint offset_tx1 = pio_add_program(pio, &ndrnkc_tx1_program);
  uint offset_rx1 = pio_add_program(pio, &ndrnkc_rx1_program);

  // SM numbers
  uint z80_tx1 = 0;
  uint z80_rx1 = 1;
  
  // Clear Z80 memory
  for (unsigned int i = 0; i < 256; ++i)
    write_io_reg (i, 0);

  // Initialize PIO for parallel interface to Z80
  z80par_program_init(pio, z80_tx1, z80_rx1,
		      offset_tx1, offset_rx1);

  // New version, set hooks to small programs that carry out the
  // required data modification
  for (unsigned int i = 0; i < 256; ++i)
    {
      // Default setup AND = 0xFF,  OR = 0x00, Affects same register
      z80_regset[i] = (uint32_t) &noaction;
      z80_regget[i] = 0;   // No data is put on bus!
    }

  // Serial card
  z80_regset[0xF0] = (uint32_t) &ser_sendflag;
  z80_regset[0xF1] = (uint32_t) &ioregwrite;
  z80_regget[0xF0] = (uint32_t) &ioregread;
  z80_regget[0xF1] = (uint32_t) &ioregread;

  // Page register
  z80_regset[0x60] = (uint32_t) &gdp_setpages;
  z80_regget[0x60] = (uint32_t) &ioregread;

  // GDP64 register set
  z80_regset[0x70] = (uint32_t) &gdp_sendcmd;
  z80_regset[0x71] = (uint32_t) &ioregwrite;
  z80_regset[0x72] = (uint32_t) &ioregwrite;
  z80_regset[0x73] = (uint32_t) &ioregwrite;
  z80_regset[0x75] = (uint32_t) &ioregwrite;
  z80_regset[0x77] = (uint32_t) &ioregwrite;
  z80_regset[0x78] = (uint32_t) &ioregwrite;
  z80_regset[0x79] = (uint32_t) &ioregwrite;
  z80_regset[0x7A] = (uint32_t) &ioregwrite;
  z80_regset[0x7B] = (uint32_t) &ioregwrite;
  for (unsigned int i = 0x70; i <= 0x7B; ++i)
    z80_regget[i] = (uint32_t) &ioregread;

  write_io_reg (0x70, 0xF4);  // Start with "non busy"

  // Keyboard
  //  z80_regset[0x68] = (uint32_t) &ioregwrite;
  z80_regget[0x69] = (uint32_t) &key_setflag;
  write_io_reg (0x68, 0x80);  // Start with "no key pending"
  z80_regget[0x68] = (uint32_t) &ioregread;
  
  // CAS
  //  z80_regset[0xCA] = (uint32_t) &ioregwrite;
  z80_regget[0xCB] = (uint32_t) &cas_setflag;
  z80_regset[0xCB] = (uint32_t) &cas_getflag;
  write_io_reg (0xCA, 0x2);   // Start with "transmit buffer empty"
  z80_regget[0xCA] = (uint32_t) &ioregread;

  //  multicore_launch_core1(core1_main);
  multicore_launch_core1_with_stack(core1_main, core1_stack, 128);
}
