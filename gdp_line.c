/**
 * gdp_line.c
 *
 * Line drawing for GDP (i.e. EF9365 chip)
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

#include <stdio.h>

#include "par_bus.h"
#include "gdp.h"
#include "gdp_line.h"


/****************************************************************************
 * Name: draw_line
 *
 * Description:
 *   Draw a line following the Bresenham algorithm as also described
 *   in the EF9365 datasheet.
 *   TODO: Line patterns (i.e. dashed, pointed, etc.) are not implemented
 *   yet.
 *
 * Input Parameters:
 *   linecode - Command for drawing the line. Interpreted according
 *              to the EF9365 datasheet.
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

void draw_line (unsigned char linecode)
{
  unsigned int x_ref = read_io_reg(gdp_xmsb) * 256 + read_io_reg(gdp_xlsb),
    y_ref = read_io_reg(gdp_ymsb) * 256 + read_io_reg(gdp_ylsb);
  int ef_dx,
    ef_dy,
    delta_x;
  int x_sign, y_sign;
  //  int D;
  int y_stride,
    two_dy,
    two_dy_dx,
    var_p;
  uint8_t gdp_dx = read_io_reg(gdp_deltax),
    gdp_dy = read_io_reg(gdp_deltay);

  // Command in which the two registers DELTAX and DELTAY
  // may be ignored by specifying the projections through
  // the CMD registers
  if (linecode & 0x80)
    {
      ef_dx = (linecode >> 5) & 0x3;
      ef_dy = (linecode >> 3) & 0x3;
    }
  else
    {
      if (linecode & 0x8)
	{
	  // Ignore the smaller of the two delta x and delta y
	  // registers, by considering it as being equal to the
	  // larger one.
	  if (gdp_dx > gdp_dy)
	    {
	      ef_dx = gdp_dx;
	      ef_dy = gdp_dx;
	    }
	  else
	    {
	      ef_dx = gdp_dy;
	      ef_dy = gdp_dy;
	    }
	}
      else
	{
	  ef_dx = gdp_dx;
	  ef_dy = gdp_dy;
	}
    }

  x_sign = (linecode & 0x2) ? 1 : 0;  // DELTAX sign, 0 if positive
  y_sign = (linecode & 0x4) ? 1 : 0;  // DELTAY sign, 0 if positive

  // Commands, which allow ignoring the DELTAX and DELTAY
  // registers by considering them as of zero value
  if ((linecode & 0x1) == 0)
    {
      // SY SX
      // 0  0 -> DELTAY ignored, DELTAX > 0
      // 0  1 -> DELTAX ignored, DELTAY > 0
      // 1  0 -> DELTAX ignored, DELTAY < 0
      // 1  1 -> DELTAY ignored, DELTAX < 0
      if (x_sign == y_sign)
	ef_dy = 0;
      else
	ef_dx = 0;
    }

  // Normal mode
  if (ef_dy > ef_dx)
    {
      y_stride = 1;
      delta_x = ef_dy;
      two_dy = ef_dx * 2;
      two_dy_dx = (ef_dx - ef_dy) * 2;
      var_p = 2 * ef_dx - ef_dy;
    }
  else
    {
      y_stride = 0;
      delta_x = ef_dx;
      two_dy = ef_dy * 2;
      two_dy_dx = (ef_dy - ef_dx) * 2;
      var_p = 2 * ef_dy - ef_dx;      
    }

  // Now draw the line
  int i = delta_x;

  while (i-- > 0)
    { 
      if (y_stride)
	y_ref += y_sign ? -1 : 1;
      else
	x_ref += x_sign ? -1 : 1;

      if (var_p < 0)
	var_p = var_p + two_dy;
      else
	{
	  if (y_stride)
	    x_ref += x_sign ? -1 : 1;
	  else
	    y_ref += y_sign ? -1 : 1;
	  var_p = var_p + two_dy_dx;
	}

      plot_pixel (x_ref, y_ref);
    }

  // Now set coordinate registers to new coordinates
  write_io_reg (gdp_xmsb, (x_ref >> 8) & 0xFF);
  write_io_reg (gdp_xlsb, x_ref & 0xFF);
  write_io_reg (gdp_ymsb, (y_ref >> 8) & 0xFF);
  write_io_reg (gdp_ylsb, y_ref & 0xFF);
}

