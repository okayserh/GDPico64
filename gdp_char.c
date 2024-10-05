/**
 * gdp_char.c
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

#include <stdio.h>

#include "par_bus.h"
#include "gdp.h"
#include "gdp_char.h"

const unsigned char charset[97][5]={{0x00,0x00,0x00,0x00,0x00},        /* ' ' whitespace */
              {0x00,0x00,0x5F,0x00,0x00},               /*  !  exclamation mark */
              {0x00,0x03,0x00,0x03,0x00},               /*  "  quotation mark */
              {0x0A,0x1F,0x0A,0x1F,0x0A},               /*  #  hash */
              {0x24,0x2A,0x7F,0x2A,0x12},               /*  $  dollar sign */
              {0x23,0x13,0x08,0x64,0x62},               /*  %  percent sign */
              {0x36,0x49,0x55,0x22,0x50},               /*  &  and sign */
              {0x00,0x00,0x0B,0x07,0x00},               /*  '  inverted comma */
              {0x00,0x1C,0x22,0x41,0x00},               /*  (  left bracket */
              {0x00,0x41,0x22,0x1C,0x00},               /*  )  right bracket */
              {0x2A,0x1C,0x7F,0x1C,0x2A},               /*  *  Asterisk */
              {0x08,0x08,0x3E,0x08,0x08},               /*  +  plus sign */
              {0x00,0x00,0xB0,0x70,0x00},               /*  ,  comma */
              {0x08,0x08,0x08,0x08,0x08},               /*  -  minus sign */
              {0x00,0x60,0x60,0x00,0x00},               /*  .  dot */
              {0x20,0x10,0x08,0x04,0x02},               /*  /  slash */
              {0x3E,0x41,0x41,0x3E,0x00},               /*  0  zero */
              {0x00,0x02,0x7F,0x00,0x00},               /*  1  one */
              {0x62,0x51,0x49,0x49,0x46},               /*  2  two */
              {0x41,0x41,0x49,0x4D,0x33},               /*  3  three */
              {0x0F,0x08,0x08,0x7F,0x08},               /*  4  four */
              {0x47,0x45,0x45,0x45,0x39},               /*  5  five */
              {0x3C,0x4A,0x49,0x49,0x30},               /*  6  six */
              {0x61,0x11,0x09,0x05,0x03},               /*  7  seven */
              {0x36,0x49,0x49,0x49,0x36},               /*  8  eight */
              {0x06,0x49,0x49,0x29,0x1E},               /*  9  nine */
              {0x00,0x36,0x36,0x00,0x00},               /*  :  colon */
              {0x00,0xB6,0x76,0x00,0x00},               /*  ;  semicolon */
              {0x08,0x14,0x22,0x41,0x00},               /*  <  less than sign */
              {0x14,0x14,0x14,0x14,0x14},               /*  =  equal sign */
              {0x00,0x41,0x22,0x14,0x08},               /*  >  greater than sign */
              {0x02,0x01,0x51,0x09,0x06},               /*  ?  question mark */
              {0x3E,0x41,0x5D,0x55,0x5E},               /*  @  at sign */
              {0x7E,0x09,0x09,0x09,0x7E},               /*  A  */
              {0x7F,0x49,0x49,0x49,0x36},               /*  B  */
              {0x3E,0x41,0x41,0x41,0x22},               /*  C  */
              {0x7F,0x41,0x41,0x41,0x3E},               /*  D  */
              {0x7F,0x49,0x49,0x49,0x41},               /*  E  */
              {0x7F,0x09,0x09,0x09,0x01},               /*  F  */
              {0x3E,0x41,0x49,0x49,0x7A},               /*  G  */
              {0x7F,0x08,0x08,0x08,0x7F},               /*  H  */
              {0x00,0x41,0x7F,0x41,0x00},               /*  I  */
              {0x20,0x40,0x40,0x40,0x3F},               /*  J  */
              {0x7F,0x08,0x14,0x22,0x41},               /*  K  */
              {0x7F,0x40,0x40,0x40,0x40},               /*  L  */
              {0x7F,0x02,0x0C,0x02,0x7F},               /*  M  */
              {0x7F,0x02,0x04,0x08,0x7F},               /*  N  */
              {0x3E,0x41,0x41,0x41,0x3E},               /*  O  */
              {0x7F,0x09,0x09,0x09,0x06},               /*  P  */
              {0x3E,0x41,0x51,0x21,0x5E},               /*  Q  */
              {0x7F,0x09,0x19,0x29,0x46},               /*  R  */
              {0x26,0x49,0x49,0x49,0x32},               /*  S  */
              {0x01,0x01,0x7F,0x01,0x01},               /*  T  */
              {0x3F,0x40,0x40,0x40,0x3F},               /*  U  */
              {0x1F,0x20,0x40,0x20,0x1F},               /*  V  */
              {0x7F,0x20,0x18,0x20,0x7F},               /*  W  */
              {0x63,0x14,0x08,0x14,0x63},               /*  X  */
              {0x07,0x08,0x70,0x08,0x07},               /*  Y  */
              {0x61,0x51,0x49,0x45,0x43},               /*  Z  */
              {0x00,0x7F,0x41,0x41,0x00},               /*  [  */
              {0x02,0x04,0x08,0x10,0x20},               /*  /  */
              {0x00,0x41,0x41,0x7F,0x00},               /*  ]  */
              {0x04,0x02,0x7F,0x02,0x04},               /*  ^  */
              /* Attention!! Please *SELECT* one of the following two lines...*DON'T* use both!!
                 9366 charset doesn't contain the _ sign...it has a left arrow instead...you can
                 choose here, wether you want the original sign or the standard ASCII one */
              {0x08,0x1C,0x2A,0x08,0x08},               /*  left arrow */
   //           {0x80,0x80,0x80,0x80,0x80},               /*  _  sign */
              /* continue with the rest of the charset */
              {0x00,0x07,0x0B,0x00,0x00},               /*  `  sign */
              {0x70,0x54,0x54,0x78,0x40},               /*  a  */
              {0x40,0x7F,0x44,0x44,0x3C},               /*  b  */
              {0x00,0x38,0x44,0x44,0x48},               /*  c  */
              {0x38,0x44,0x44,0x7F,0x40},               /*  d  */
              {0x00,0x38,0x54,0x54,0x48},               /*  e  */
              {0x00,0x08,0x7C,0x0A,0x02},               /*  f  */
              {0x00,0x98,0xA4,0xA4,0x7C},               /*  g  */
              {0x00,0x7F,0x04,0x04,0x78},               /*  h  */
              {0x00,0x00,0x7A,0x00,0x00},               /*  i  */
              {0x00,0x40,0x80,0x74,0x00},               /*  j  */
              {0x00,0x7E,0x10,0x28,0x44},               /*  k  */
              {0x00,0x02,0x7E,0x40,0x00},               /*  l  */
              {0x7C,0x04,0x7C,0x04,0x78},               /*  m  */
              {0x00,0x7C,0x04,0x04,0x78},               /*  n  */
              {0x00,0x38,0x44,0x44,0x38},               /*  o  */
              {0x00,0xFC,0x24,0x24,0x18},               /*  p  */
              {0x18,0x24,0x24,0xFC,0x80},               /*  q  */
              {0x00,0x7C,0x08,0x04,0x04},               /*  r  */
              {0x00,0x48,0x54,0x54,0x24},               /*  s  */
              {0x00,0x04,0x3E,0x44,0x20},               /*  t  */
              {0x3C,0x40,0x40,0x7C,0x40},               /*  u  */
              {0x0C,0x30,0x40,0x30,0x0C},               /*  v  */
              {0x3C,0x40,0x30,0x40,0x3C},               /*  w  */
              {0x44,0x24,0x38,0x48,0x44},               /*  x  */
              {0x00,0x1C,0x20,0xA0,0xFC},               /*  y  */
              {0x40,0x64,0x54,0x4C,0x04},               /*  z  */
              {0x00,0x08,0x36,0x41,0x41},               /*  {  */
              {0x00,0x00,0x77,0x00,0x00},               /*  |  */
              {0x00,0x41,0x41,0x36,0x08},               /*  }  */
              /* Attention!! Please *SELECT* one of the following two lines...*DON'T* use both!!
                 9366 charset doesn't contain the ~ sign...it has a strange hook instead...you can
                 choose here, wether you want the original sign or the standard ASCII one */
              {0x08,0x08,0x08,0x08,0x38},               /*  strange hook */
    //          {0x18,0x04,0x08,0x10,0x0C},              /*  ~ sign */
              /* and the last sign */
              {0x55,0x2A,0x55,0x2A,0x55},                /* "ralley" sign */
              {0xFF,0xFF,0xFF,0xFF,0xFF}                 /* 5x8 block for block drawing */
             };


unsigned int x_cnt,
  y_cnt,
  xs_cnt,
  ys_cnt,
  pix_cnt = 0;


/****************************************************************************
 * Name: draw_char
 *
 * Description:
 *   Draws a character according to the description in the EF9365
 *   datasheet. Implements skewed and scaled characters.
 *   TODO: 5x5 Block drawing not yet implemented!
 *
 * Input Parameters:
 *   c     - As provided in the respective GDP command. Usually
 *           the ASCII character
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

void draw_char (unsigned char a)
{
  // Limit to valid values
  unsigned char c = ((a < 0x20u) || (a > 0x80u)) ? 0x0 : (a-0x20u);
  // Get pointer into data array
  const unsigned char *data = &(charset[c][0]);

  // Initiation of drawing action
  if (pix_cnt == 0)
    {
    }

  uint8_t ctrlreg1 = read_io_reg (gdp_ctrl1);
  uint8_t ctrlreg2 = read_io_reg (gdp_ctrl2);

  uint8_t siz = read_io_reg (gdp_csize);
  unsigned int size_x = (siz & 0xF0) >> 4,
    size_y = (siz & 0xF);
  if (size_x == 0)
    size_x = 16;
  if (size_y == 0)
    size_y = 16;

  unsigned int x_ref = read_io_reg(gdp_xmsb) * 256 + read_io_reg(gdp_xlsb),
    y_ref = read_io_reg(gdp_ymsb) * 256 + read_io_reg(gdp_ylsb);

  // For debugging, define reference point
  // plot_pixel (x_ref, y_ref);

  int x_plot, y_plot,
    pos_buf = (ctrlreg2 & 0x8) ? y_ref : x_ref;

  if (ctrlreg1 & 0x1)
    {
      for (unsigned int i = 0; i < 5; ++i)
	{
	  unsigned char ldat = *data;
	  for (unsigned int isi = 0; isi < size_x; ++isi)
	    {
	      x_plot = pos_buf;
	      y_plot = (ctrlreg2 & 0x8) ? x_ref : y_ref;
	      
	      for (unsigned int j = 0; j < 8; ++j)
		{
		  for (unsigned int jsi = 0; jsi < size_y; ++jsi)
		    {
		      if (ldat & (0x80 >> j))
			{
			  if (ctrlreg2 & 0x8)
			    plot_pixel (y_plot, x_plot);
			  else
			    plot_pixel (x_plot, y_plot);
			}
		      
		      ++y_plot;
		      if (ctrlreg2 & 0x4)
			x_plot += (ctrlreg2 & 0x8) ? -1 : 1;
		    }
		}
	      
	      pos_buf += (ctrlreg2 & 0x8) ? -1 : 1;
	    }
	  data++;
	}
    }

  // Adjust position register (From gdp64.c nkcemu)
  if (ctrlreg2 & 0x8)
    {
      y_ref += 6*size_x;
      write_io_reg (gdp_ymsb, (y_ref >> 8) & 0xFF);
      write_io_reg (gdp_ylsb, y_ref & 0xFF);
    }
  else
    {
      x_ref += 6*size_x;
      write_io_reg (gdp_xmsb, (x_ref >> 8) & 0xFF);
      write_io_reg (gdp_xlsb, x_ref & 0xFF);
    }    

  /*
  pos_buf += size_x;
  if (ctrlreg2 & 0x8)
    {
      gdp64->ymsb = (pos_buf >> 8) & 0xFF;
      gdp64->ylsb = pos_buf & 0xFF;
    }
  else
    {
      gdp64->xmsb = (pos_buf >> 8) & 0xFF;
      gdp64->xlsb = pos_buf & 0xFF;
    }
  */
}

/****************************************************************************
 * Name: test_draw_char
 *
 * Description:
 *   Some test code to verify the correct operation of the
 *   character routines.
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

void test_draw_char ()
{
  /*
  gdp64->xmsb = 0;
  gdp64->xlsb = 50;
  gdp64->ymsb = 0;
  gdp64->ylsb = 200;
  gdp64->csize = 0x11;  // Standard size
  draw_char ('A');

  gdp64->xlsb += 20;
  gdp64->ctrl2 |= 0x8;
  draw_char ('A');

  gdp64->xlsb += 20;
  gdp64->ctrl2 |= 0x4;
  draw_char ('A');

  gdp64->xlsb += 20;
  gdp64->ctrl2 &= ~0x8;
  draw_char ('A');

  // Test positioning
  gdp64->ylsb = 100;
  gdp64->xlsb = 50;
  gdp64->ctrl2 = 0;
  draw_char ('B');
  draw_char ('B');

  gdp64->csize = 0x22;  // Standard size
  gdp64->ctrl2 = 0;
  gdp64->xlsb += 20;
  draw_char ('A');

  gdp64->xlsb += 40;
  gdp64->ctrl2 = 0x4;
  draw_char ('A');

  // Draw a block
  gdp64->xlsb += 40;
  gdp64->ctrl2 = 0x4;
  draw_char (97 + 0x20);
  */
  // Hardcore test
  for (unsigned char i = 0x20; i < 0x82; ++i)
    draw_char (i);
  
}
