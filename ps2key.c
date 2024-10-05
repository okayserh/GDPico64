/**
 * ps2key.c
 *
 * Implements a basic PS/2 Keyboard interface. Based on project
 * ps2kbd-lib
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

#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/pio.h"
#include "hardware/irq.h"
#include "ps2key.pio.h"

#include "ps2key.h"

#include <stdio.h>

uint sm_ps2key = 0;

PIO kbd_pio = pio1;

StaticQueue_t ps2QueueBuffer;

#define PS2_QUEUE_LENGTH 16
#define PS2_QUEUE_IS 1
uint8_t ucPS2QueueStorage[PS2_QUEUE_LENGTH * PS2_QUEUE_IS];

CharDev ps2_dev;

// clang-format off

#define BS 0x8
#define TAB 0x9
#define LF 0xA
#define ESC 0x1B

// Upper-Case ASCII codes by keyboard-code index, 16 elements per row
static const uint8_t lower[] = {
    0,  0,   0,   0,   0,   0,   0,   0,  0,  0,   0,   0,   0,   TAB, '`', 0,
    0,  0,   0,   0,   0,   'q', '1', 0,  0,  0,   'z', 's', 'a', 'w', '2', 0,
    0,  'c', 'x', 'd', 'e', '4', '3', 0,  0,  ' ', 'v', 'f', 't', 'r', '5', 0,
    0,  'n', 'b', 'h', 'g', 'y', '6', 0,  0,  0,   'm', 'j', 'u', '7', '8', 0,
    0,  ',', 'k', 'i', 'o', '0', '9', 0,  0,  '.', '/', 'l', ';', 'p', '-', 0,
    0,  0,   '\'',0,   '[', '=', 0,   0,  0,  0,   LF,  ']', 0,   '\\',0,   0,
    0,  0,   0,   0,   0,   0,   BS,  0,  0,  0,   0,   0,   0,   0,   0,   0,
    0,  0,   0,   0,   0,   0,   ESC, 0,  0,  0,   0,   0,   0,   0,   0,   0};

// Upper-Case ASCII codes by keyboard-code index
static const uint8_t upper[] = {
    0,  0,   0,   0,   0,   0,   0,   0,  0,  0,   0,   0,   0,   TAB, '~', 0,
    0,  0,   0,   0,   0,   'Q', '!', 0,  0,  0,   'Z', 'S', 'A', 'W', '@', 0,
    0,  'C', 'X', 'D', 'E', '$', '#', 0,  0,  ' ', 'V', 'F', 'T', 'R', '%', 0,
    0,  'N', 'B', 'H', 'G', 'Y', '^', 0,  0,  0,   'M', 'J', 'U', '&', '*', 0,
    0,  '<', 'K', 'I', 'O', ')', '(', 0,  0,  '>', '?', 'L', ':', 'P', '_', 0,
    0,  0,   '"', 0,   '{', '+', 0,   0,  0,  0,   LF,  '}', 0,   '|', 0,   0,
    0,  0,   0,   0,   0,   0,   BS,  0,  0,  0,   0,   0,   0,   0,   0,   0,
    0,  0,   0,   0,   0,   0,   ESC, 0,  0,  0,   0,   0,   0,   0,   0,   0};
// clang-format on

static uint8_t release; // Flag indicates the release of a key
static uint8_t shift;   // Shift indication
static uint8_t ascii;   // Translated to ASCII


/****************************************************************************
 * Name: ps2key_irq_handler
 *
 * Description:
 *   Interrupt handler for the PIO interrupt triggered by the PS/2
 *   PIO when a full character has been received.
 *   Uses some kind of state machine to decode the data into characters
 *   and conveys those to a FreeRTOS queue.
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

void __not_in_flash_func(ps2key_irq_handler)(void)
{
  BaseType_t xHigherPriorityTaskWoken;

  // Pull and process data from PIO
  while (!pio_sm_is_rx_fifo_empty(kbd_pio, sm_ps2key))
    {
      // pull a scan code from the PIO SM fifo
      uint8_t code = *((io_rw_8*)&kbd_pio->rxf[sm_ps2key] + 3);
      switch (code) {
      case 0xF0:               // key-release code 0xF0 detected
        release = 1;         // set release
        break;               // go back to start
      case 0x12:               // Left-side SHIFT key detected
      case 0x59:               // Right-side SHIFT key detected
        if (release) {       // L or R Shift detected, test release
	  shift = 0;       // Key released preceded  this Shift, so clear shift
	  release = 0;     // Clear key-release flag
        } else
	  shift = 1; // No previous Shift detected before now, so set Shift_Key flag now
        break;
      default:
        // no case applies
        if (!release)                              // If no key-release detected yet
	  ascii = (shift ? upper : lower)[code]; // Get ASCII value by case
        release = 0;
        break;
      }

      // If we have a character, put it into Queue
      if (ascii) 
	{
	  xQueueSendFromISR (ps2_dev.input_queue, &ascii, &xHigherPriorityTaskWoken);
	  ascii = 0;
	}
    }
}

/****************************************************************************
 * Name: init_ps2key
 *
 * Description:
 *   Initializes the state machine for the PS/2 interface and
 *   creates a FreeRTOS queue to receive the characters from the
 *   keyboard.
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

void init_ps2key ()
{
  // PS2 Keyboard is placed on PIO1.
  PIO pio = pio1;
  uint offset_ps2kbd = pio_add_program(pio, &ps2key_program);
  ascii = 0;
  
  // Initialize Queue to take up the keyboard events
  ps2_dev.input_queue = xQueueGenericCreateStatic(PS2_QUEUE_LENGTH,
						  PS2_QUEUE_IS,
						  &(ucPS2QueueStorage[0]),
						  &ps2QueueBuffer,
						  queueQUEUE_TYPE_BASE);
  ps2_dev.output_queue = NULL;
  ps2_dev.input_task = NULL;
  ps2_dev.output_task = NULL;
  
  // Set interrupt routine for ps2 keyboard
  irq_set_exclusive_handler(PIO1_IRQ_0, ps2key_irq_handler);
  irq_set_enabled(PIO1_IRQ_0, true);
  
  ps2key_program_init(pio, sm_ps2key, offset_ps2kbd);
}
