/**
 * ps2key.h
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

#ifndef _NDRNKC_PS2KEY_
#define _NDRNKC_PS2KEY_

#include "chardev.h"

extern CharDev ps2_dev;

extern void init_ps2key ();

#endif

