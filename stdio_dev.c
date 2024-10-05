/**
 * stdio_dev.c
 *
 * Wraps the "chardev" interface to the stdio (via
 * USB or UART) as provided by the rp2040 sdk.
 *
 * Copyright (C) 1883  Thomas Edison (?!)
 * Copyright (C) 2024  Oliver Kayser-Herold (Adaptations of use in project)
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

#include "chardev.h"

#include "pico/stdlib.h"

/* Definitions for the input- and output queue */
#define STDIO_IN_QUEUE_LENGTH 32
#define STDIO_IN_IS 1
StaticQueue_t stdio_in_queue_buf;
uint8_t ucSTDIOINQueueStorage[STDIO_IN_QUEUE_LENGTH * STDIO_IN_IS];

#define STDIO_OUT_QUEUE_LENGTH 32
#define STDIO_OUT_IS 1
StaticQueue_t stdio_out_queue_buf;
uint8_t ucSTDIOOUTQueueStorage[STDIO_OUT_QUEUE_LENGTH * STDIO_OUT_IS];

/* The index within the input task's array of task notifications
   to use. */
const UBaseType_t xArrayIndex = 1;

/* CharDev provding the input/output queues for the stdio interface */

CharDev stdio_dev;


/****************************************************************************
 * Name: keyb_handler
 *
 * Description:
 *   Callback function for the pico sdk keyboard handler
 *
 * Input Parameters:
 *   parm   - As provided to the initiatlization of the callback. Should
 *            be the respective FreeRTOS Queue
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

void __not_in_flash_func(keyb_handler)(void *parm)
{
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  configASSERT (stdio_dev.input_task != NULL);

  vTaskNotifyGiveIndexedFromISR (stdio_dev.input_task,
				 xArrayIndex,
				 &xHigherPriorityTaskWoken);
}

/****************************************************************************
 * Name: stdio_out_monitor
 *
 * Description:
 *   Listens on the output queue for characters. When a character
 *   is received, it is written to the serial port via the pico sdk.
 *
 * Input Parameters:
 *   unused_arg   - Currently unused
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

void stdio_out_monitor(void* unused_arg) {
  uint8_t ch;
  while (1)
    {
      if (xQueueReceive (stdio_dev.output_queue, &ch, (TickType_t) 10))
	{
	  putchar_raw (ch);
	}
    }
}

/****************************************************************************
 * Name: stdio_in_monitor
 *
 * Description:
 *   Listens on for a task notification from the pico stdio callback
 *   function. When the notification is received, all characters are read
 *   from pico stdio and written to the input queue.
 *
 * Input Parameters:
 *   unused_arg   - Currently unused
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

void stdio_in_monitor(void* unused_arg) {
  uint8_t ch;
  uint32_t ulNotificationValue;
  const TickType_t xMaxBlockTime = pdMS_TO_TICKS( 200 );

  while (1)
    {
      // Wait for notification from stdio callback
      ulNotificationValue = ulTaskNotifyTakeIndexed(xArrayIndex,
						    pdTRUE,
						    xMaxBlockTime);
      // If the right notification was received,
      // get all available chars from stdio and
      // put them to the input queue (blocking!)
      if (ulNotificationValue == 1)
	{
	  int ch = getchar_timeout_us (1);
	  while (ch >= 0)
	    {
	      uint8_t chq = (uint8_t) ch;
	      xQueueSend (stdio_dev.input_queue,
			  &chq,
			  portMAX_DELAY);
	      ch = getchar_timeout_us (1);
	    }
	}
    }
}

/****************************************************************************
 * Name: init_stdio_dev
 *
 * Description:
 *   Initializes the queues for the stdio device. Registers
 *   callback for received characters from pico sdk and
 *   starts task for transmitting characters to the USB/UART
 *   via the pico sdk.
 *
 * Input Parameters:
 *   parm   - None
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

void init_stdio_dev ()
{
  // Initialize Queue to take up the keyboard events
  stdio_dev.input_queue = xQueueGenericCreateStatic(STDIO_IN_QUEUE_LENGTH,
					     STDIO_IN_IS,
					     &(ucSTDIOINQueueStorage[0]),
					     &stdio_in_queue_buf,
					     queueQUEUE_TYPE_BASE);

  stdio_dev.output_queue = xQueueGenericCreateStatic(STDIO_OUT_QUEUE_LENGTH,
					      STDIO_OUT_IS,
					      &(ucSTDIOOUTQueueStorage[0]),
					      &stdio_out_queue_buf,
					      queueQUEUE_TYPE_BASE);

  /* Setup a task which outputs characters
     received from the queue to the pico stdout */
  BaseType_t monitor_status = xTaskCreate(stdio_out_monitor,
					  "STDIO_OUT_TASK", 
					  128,
					  NULL, 
					  1,
					  &(stdio_dev.output_task));

  monitor_status = xTaskCreate(stdio_in_monitor,
			       "STDIO_IN_TASK", 
			       128,
			       NULL, 
			       1,
			       &(stdio_dev.input_task));

  /*
   * Set callback to get characters from stdio
   */
  stdio_set_chars_available_callback (keyb_handler, stdio_dev.input_queue);
}
