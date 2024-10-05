/**
 * chardev.h
 *
 * Generic interface for character device
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

#ifndef _NDRNKC_CHARDEV_
#define _NDRNKC_CHARDEV_

// FreeRTOS
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>

/*
Every device opens one or more queues which
are fed from the FIFO. I.e. the assembler snippets
handling the IO access must put the request onto
the intercore FIFO. In the interrupt, the requested
action is put onto the corresponding queue for that device.
*/

typedef struct CharDev_t
{
  // Queue on which other tasks can listen for characters
  QueueHandle_t input_queue;

  // Queue to which other tasks can send characters
  QueueHandle_t output_queue;

  // Link to task which handles the input
  TaskHandle_t input_task;

  // Link to task which handles the output
  TaskHandle_t output_task;
} CharDev;

#endif
