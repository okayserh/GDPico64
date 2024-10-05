/**
 * cas.h
 *
 * Functions for the CAS interface board. (I.e. in
 * essence a 6850 UART Chip)
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

#ifndef _NDRNKC_CAS_
#define _NDRNKC_CAS_

#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>

// IO handling in assembler
extern void cas_getflag (void);
extern void cas_setflag (void);

// Cassette interface
extern QueueHandle_t cas_queue;

extern void init_cas ();

#endif
