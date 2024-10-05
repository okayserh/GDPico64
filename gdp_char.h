/**
 * gdp_char.h
 *
 * Character generation for GDP (i.e. EF9365 chip).
 *
 * Copyright (C) 2007  Torsten Evers (tevers@onlinehome.de) (Character map)
 * Copyright (C) 2024  Oliver Kayser-Herold (Implementation
 * and project integration)
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

#ifndef _NDRNKC_GDP_CHAR_
#define _NDRNKC_GDP_CHAR_

#include "pico/stdlib.h"

extern void draw_char (unsigned char a);

extern void test_draw_char ();

#endif
