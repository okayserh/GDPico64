/**
 * stdio_dev.h
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

#ifndef _NDRNKC_STDIO_DEV_
#define _NDRNKC_STDIO_DEV_

#include "chardev.h"

extern CharDev stdio_dev;

void init_stdio_dev ();

#endif
