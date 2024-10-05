/**
 * parport.S
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

decl_func parloop

	//  Base address of const table in r0
	//  Base adress of memory in r7
	//  Base adress PIO in r6
	adr r3, const_table
	ldr r4, [r3, #4]	// Check for PIO_FSTAT_RXEMPTY rx1
	ldr r6, [r3, #8]	// PIO0 base address
	ldr r7, [r3, #12]	// SPINLOCK2 register address

	// Save base address of z80_reg_set in r9
	mov r9, r1
	// Base address of z80_reg_get in r8 (r9 + 1024)
	movs r3, #1
	lsls r3, #10  // r3 = 1024
	adds r1, r3
	mov r8, r1

.global coreloop
coreloop:
1:
	ldr r1, [r6, #4]  	// PIO0->FSTAT in r1

	// Test for bit 8 in PIO0->FSTAT
	movs r5, #1
	lsls r5, #8
	tst r1, r5
	bne 2f

	// Wait for free Spinlock and block
6:
	ldr r5, [r7, #0]
	cmp r5, #0
	beq 6b

	// Debug counter. Increment every time a read event is registered
	ldr r5, [r0, #0]
	adds r5, #1
	str r5, [r0, #0]

	//  TX1 not empty, Read cycle
	ldr r1, [r6, #32]	// PIO0->RXF0 in r1

	// Jump to snippet for this register
	movs r2, r1
	lsls r2, #2   // Shift left 2 to get Jump offset (*4)

	mov r5, r8
	ldr r5, [r5, r2]
	bx r5  // Jump to some place r0 = Memory/IO area, r1 = IO Adress 0x00 to 0xFF, r2 = IO Address * 4
	nop

.align 4
const_table:
	.word 0x00000100	// PIO_FSTAT_RXEMPTY_LSB + z80_tx1 (0)
	.word 0x00000200	// PIO_FSTAT_RXEMPTY_LSB + z80_rx1 (1)
	.word 0x50200000	// PIO0_BASE
	.word 0xd0000104	// SPINLOCK2  (used to lock register bank)

decl_func ioregread
	// Output to data bus
	ldrb r2, [r0, r1]       // r2 = z80_mem[A0-A7]
	str r5, [r7, #0]       	// "Writing (any value) releases the lock"
	str r2, [r6, #16]   	// PIO->TXF0 from r2

	b 1b
	nop

2:
	// RX1 not empty, Z80 Write cycle
	tst r1, r4
	bne 1b

	// Wait for free Spinlock and block
7:
	ldr r5, [r7, #0]
	cmp r5, #0
	beq 7b

	// Debug counter. Increment every time a read event is registered
	ldr r5, [r0, #4]
	adds r5, #1
	str r5, [r0, #4]

	// RX1 not empty, Read cycle
	// in pins, 8   A0-A7
	ldr r1, [r6, #36]	// PIO0->RXF1 in r1 (A0-A7)
	lsls r1, #2   // Shift left 2 to get Jump offset (*4)

	ldr r2, [r6, #36]	// PIO0->RXF1 in r1 (D0-D7)

	mov r5, r9
	ldr r5, [r5, r1]
	bx r5  // Jump to some place r0 = Memory/IO area, r1 = IO Address * 4, r2 = Data
	nop

	// Default implementation, writes the received data into IO register
decl_func ioregwrite
	lsrs r1, #2   //  Shift right 2 bits to get offset
	strb r2, [r0, r1]

	// Default implementation, just do nothing.
decl_func noaction
3:
	// "Writing (any value) releases the lock"
	str r5, [r7, #0]

	movs r5, #1  // Set PIO IRQ 0 to signal the PIO to continue
	str r5, [r6, #52]

	b 1b
	nop

	// TODO: Move to separate file
	// Special treatment for serial port. Set flag in status register
	// and then put data into FIFO
decl_func ser_sendflag
	// Move Status register (F1) into R3
	lsrs r1, #2  // Should be F0
	adds r1, #1  // Move to F1 (Status register)
	ldrb r3, [r0, r1]

	movs r5, #16
	tst r3, r5
	bne 4f       // When the flag is already set, do nothing
	// Else set flag and inform FIFO
	orrs r3, r5
	strb r3, [r0, r1]

	subs r1, #1
	lsls r1, #8
	orrs r1, r2

	// Put original PIO data to FIFO
	adr r2, const_ser
	ldr r2, [r2, #0]
	str r1, [r2, #0]
4:
	b 3b
	nop

.align 4
const_ser:
	.word 0xD0000054  // FIFO_WR
