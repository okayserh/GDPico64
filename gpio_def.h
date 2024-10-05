/**
 * gpio_def.h
 *
 * Defines the used GPIOs (must match the GDPico64 hardware
 * design).
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

#ifndef _NDRNKC_GPIO_DEF_
#define _NDRNKC_GPIO_DEF_

#define KEY_DATA        0
#define KEY_CLK         1

#define VGA_BLUE_D1     2
#define VGA_BLUE_D2     3
#define VGA_GREEN_D0    4
#define VGA_GREEN_D1    5
#define VGA_GREEN_D2    6
#define VGA_RED_D0      7
#define VGA_RED_D1      8
#define VGA_RED_D2      9
#define VGA_CSYNC      10

#define NOT_RD         11
#define A0             12
#define A1             13
#define A2             14
#define A3             15
#define A4             16
#define A5             17
#define A6             18
#define A7             19
#define NOT_OE_A       20
#define NOT_OE_D       21
#define NOT_WAIT       22
#define NOT_WR         26
#define NOT_IOR        27

#define RESET          28

#endif
