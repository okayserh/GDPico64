/**
 * gdp_io.S
 *
 * Functions for the parallel side of the GDP graphics card
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

decl_func gdp_sendcmd
	lsrs r1, #2  // Should be 70
	ldrb r3, [r0, r1]  // Status register is same as CMD reg
	movs r5, #4
	tst r3, r5
	beq noaction       // When the flag is low, do nothing
	// Else set flag and inform FIFO
	//	rsbs r5, #0  // Should be ~4. Seems not to work
	movs r5, #3  // Test for debugging	
	ands r3, r5
	strb r3, [r0, r1]

	// Put A0-A7,D0-D7 to Interprocesor FIFO
	lsls r1, #8
	orrs r1, r2

	adr r2, const_gdp
	ldr r2, [r2, #0]
	str r1, [r2, #0]

	b noaction

.align 4
const_gdp:
	.word 0xD0000054  // FIFO_WR

decl_func gdp_setpages
	lsrs r1, #2   // Store in register
	strb r2, [r0, r1]

	// Put A0-A7,D0-D7 to Interprocesor FIFO
	lsls r1, #8
	orrs r1, r2

	adr r2, const_gdp_
	ldr r2, [r2, #0]
	str r1, [r2, #0]

	b noaction

.align 4
const_gdp_:
	.word 0xD0000054  // FIFO_WR
