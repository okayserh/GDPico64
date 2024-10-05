/**
 * key_io.S
 *
 * Functions to implement the KEY group, which
 * (originally) implements a parallel keyboard interface.
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

#include "hardware/regs/addressmap.h"
#include "hardware/regs/sio.h"
#include "hardware/regs/pio.h"

.syntax unified
.cpu cortex-m0plus
.thumb


.macro decl_func_x name
.section .scratch_x.\name, "ax"
.global \name
.type \name,%function
.thumb_func
\name:
.endm

#define decl_func decl_func_x
	
	// Special treatment for keyboard
	// Reading 0x69 resets the strobe pulse to 1
decl_func key_setflag
	subs r1,#1  // R1 initially 0x69, -1 to get to 0x68
	ldrb r3, [r0, r1]

	// Set bit 7 in register
	movs r5, #128
	orrs r3, r5
	strb r3, [r0, r1]

	// Output to data bus
	adds r1,#1          // r1 = 0x69 again
	ldrb r2, [r1, r0]   // r2 = z80_mem[A0-A7]

	// "Writing (any value) releases the lock"
	str r5, [r7, #0]

	str r2, [r6, #16]   	// PIO->TXF0 from r2

	// Return to coreloop
	b coreloop

.align 4
retadr:
	.word coreloop
