/**
 * gdp.h
 *
 * Support functions for the VGA output, simulating
 * an EF9365 chip.
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

#ifndef _NDRNKC_GDP_
#define _NDRNKC_GDP_

#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>

// Queue for receiving commands
extern QueueHandle_t gdp_queue;
extern QueueHandle_t gdp_page_queue;

// IO handling in assembler
extern void gdp_sendcmd (void);
extern void gdp_setpages (void);

extern int init_gdp ();

extern void plot_pixel (int x, int y);
extern void clear_pixel (int x, int y);

extern void gdp_proc_command (unsigned char gdp_cmd);
extern void gdp_set_pages (unsigned int r_page, unsigned int w_page);

#define GDP_BASE 0x70

#define gdp_status  (GDP_BASE)
#define gdp_ctrl1   (GDP_BASE + 1)
#define gdp_ctrl2   (GDP_BASE + 2)
#define gdp_csize   (GDP_BASE + 3)
#define gdp_deltax  (GDP_BASE + 5)
#define gdp_deltay  (GDP_BASE + 7)
#define gdp_xmsb    (GDP_BASE + 8)
#define gdp_xlsb    (GDP_BASE + 9)
#define gdp_ymsb    (GDP_BASE + 10)
#define gdp_ylsb    (GDP_BASE + 11)
#define gdp_xlp     (GDP_BASE + 12)
#define gdp_ylp     (GDP_BASE + 13)

#endif
