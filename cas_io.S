/**
 * cas_io.S
 *
 * Functions for the parallel side of the CAS interface.
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

	// Special treatment for serial port. Set flag in status register
	// and then put data into FIFO
decl_func cas_getflag
	// Move Status register (F1) into R3
	lsrs r1, #2  // Should be CB (Data
	subs r1, #1  // Move to CA (Cmd/Status register)
	ldrb r3, [r0, r1]

	movs r5, #2
	tst r3, r5
	beq noaction       // When the flag is clear, do nothing
	// Else clear flag and inform FIFO
	movs r5, #1

	ands r3, r5
	strb r3, [r0, r1]

	adds r1, #1
	lsls r1, #8
	orrs r1, r2

	// Put original PIO data to FIFO
	adr r2, const_cas
	ldr r2, [r2, #0]
	str r1, [r2, #0]

	b noaction

.align 4
const_cas:
	.word 0xD0000054  // FIFO_WR

	// Special treatment for serial port. Set flag in status register
	// and then put data into FIFO
decl_func cas_setflag
	// Move Status register (F1) into R3
	subs r1, #1  // Move to CA (Cmd/Status register)
	ldrb r3, [r0, r1]

	// Clear bit 1 in register CA
	movs r5, #2
	ands r3, r5
	strb r3, [r0, r1]

	adds r1, #2  // Move to CC (used as receive data register)
	ldrb r2, [r1, r0]

	// "Writing (any value) releases the lock"
	str r5, [r7, #0]
	str r2, [r6, #16]   	// PIO->TXF0 from r2

	b coreloop
