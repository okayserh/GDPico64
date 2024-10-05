/**
 * gdp.c
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

#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "gdp.pio.h"
#include <stdio.h>
#include <string.h>

#include "par_bus.h"
#include "gdp.h"
#include "gdp_char.h"
#include "gdp_line.h"


uint sm_gdp_sync = 2;
uint sm_gdp_data = 3;
uint sm_gdp_lut = 1;

uint dma_channel_0;             // DMA channel for transferring sync data to PIO
uint dma_channel_1;             // DMA channel for transferring pixel data data to PIO

/*
 * Conceptual ideas of the VGA implementation:
 *
 * The synchronization signal is a sequence of pulses of well defined lengths. The
 * PIO implementation for generating the sync signals is currently rather minimalistic.
 * It gets a 16 bit word from DMA with the following structure:
 * 1 Bit -> Signal to output (high/low)
 * 10 Bit -> Counter to define the pulse length
 * 5 Bit -> Jump destination. This needs to be aligned with the PIO assembler
 * code (by defining preprocessor variables PIO_JMP_NO_DATA and PIO_JMP_START_DATA).
 * One possibility is to jump directly into the loop for counting down
 * the pulse length. The other option is to jump to an instruction that sets
 * PIO IRQ4 to trigger the data statemachine.
 *
 * With these 16 bit chunks the required SYNC signal for a line is constructed.
 *
 * Those chunks are assembled into a complete display list (function calc_fulldlist),
 * which is transferred to the PIO via DMA for each frame. This concept consumes
 * some memory for the displaylist (~16k), but does only require infrequent
 * attention of the CPU.
 *
 * Preprocessor macro "COMPDARK" helps to create the PIO chunks by encapsulating
 * the required bit manipulations.
 *
 * In the code below three SYNC patterns are defined. One recreates the original
 * timing of the EF9365. This was a first attempt to get video out of the
 * RP2040 and works generally. However, for the final PCB design the aim was
 * to have a more modern VGA connector. Therefore, the code fragments are only
 * left as reference to how it should look like to recreate the original EF3965.
 *
 * The second SYNC pattern defines the standard VGA (640x480, 60 Hz) resolution.
 * Timing should be OK. However, the frequency divider for the PIO would need to
 * be adjusted to make this work again.
 *
 * Last option implements the timing for 1920x1080P @ 60Hz. This requires slight
 * overclocking of the PICO to 148.5 MHz. Out of spec, but haven't observed any
 * problems with this frequency. This is the required pixel clock for the 1920x1080P@60Hz.
 *
 * Since the PICO memory is limited and the original GPD64 only requires a screen resolution
 * of 512x256 pixels, the following mapping is implemented in the PIO.
 * Each horizontal Pixel is tripled by introducing one wait cycle in the PIO. This
 * results in 1536 used Pixels with the rest being black/background.
 * Each vertical Pixel is quadrupled to achive 1024 Pixels. This fits well into
 * the available 1080 pixels.
 */


/// Note: 1-sig is to invert the SYNC signal. I.e. when it is low, the 0V Sync is output
#define COMPDARK(sig,dur,next) ( ((uint32_t)(1-sig) << 15) | ((uint32_t)(dur)<<5) | (uint32_t)(next) )

/*
5 times vertical sync
5 times post-equalizing (6 times for odd frames)
1 times post-equalizing empty
304 line_sync (one with pulse, one without pulse, fmat = 1)
303 line_sync (fmat = 0)
5 times pre-equalizing  (6 times for odd frames)

pointer, length, count
*/

typedef struct displist_s
{
  uint32_t *lsync;
  uint32_t len;
  uint32_t count;
} displist;

#define PIO_JMP_START_DATA 14
#define PIO_JMP_NO_DATA 15

uint32_t vert_sync = COMPDARK(1,413,PIO_JMP_NO_DATA) * 65536 + COMPDARK(0,29,PIO_JMP_NO_DATA);
uint32_t equalize_sync = COMPDARK(1,29,PIO_JMP_NO_DATA) * 65536 + COMPDARK(0,413,PIO_JMP_NO_DATA);
uint32_t equalize_emt = COMPDARK(0,29,PIO_JMP_NO_DATA) * 65536 + COMPDARK(0,413,PIO_JMP_NO_DATA);
uint32_t line_sync_bl[2] = {
  COMPDARK(1,61,PIO_JMP_NO_DATA) * 65536 + COMPDARK(0,189,PIO_JMP_NO_DATA),
  COMPDARK(0,317,PIO_JMP_NO_DATA) * 65536 + COMPDARK(0,317,PIO_JMP_NO_DATA),
};
uint32_t line_sync_dt[2] = {
  COMPDARK(1,61,PIO_JMP_NO_DATA) * 65536 + COMPDARK(0,189,PIO_JMP_NO_DATA),
  COMPDARK(0,317,PIO_JMP_START_DATA) * 65536 + COMPDARK(0,317,PIO_JMP_NO_DATA),
};

// Original EF9365 timing (896 clocks per line)
// 416+32 = 448 * 2 = 896
// 64+192+320+320 = 640,704,896
/*
  // Each VGA Line (800 clocks per line)
160 + 640 = 800

  // Normal line sync
96 1
704 0

  // Vertical blank
96 0
704 1
*/

// http://martin.hinner.info/vga/timing.html

uint32_t vga_vert_sync = COMPDARK(0,93,PIO_JMP_NO_DATA) * 65536 + COMPDARK(1,704,PIO_JMP_NO_DATA);
uint32_t vga_line_sync_bl[2] = {
  COMPDARK(1, 93,PIO_JMP_NO_DATA) * 65536 + COMPDARK(0,45,PIO_JMP_NO_DATA),
  COMPDARK(0,637,PIO_JMP_NO_DATA) * 65536 + COMPDARK(0,13,PIO_JMP_NO_DATA),
};
uint32_t vga_line_sync_dt[2] = {
  COMPDARK(1, 93,PIO_JMP_NO_DATA) * 65536 + COMPDARK(0,45,PIO_JMP_NO_DATA),
  COMPDARK(0,636,PIO_JMP_START_DATA) * 65536 + COMPDARK(0,13,PIO_JMP_NO_DATA),
};


// https://projectf.io/posts/video-timings-vga-720p-1080p/#hd-1920x1080-30-hz
// 2200 Total Pixels, we let the SM run with one quarter pixel freq to stay within 10 bits counter
// 2200 / 4

uint32_t hd_vert_sync = COMPDARK(0,8,PIO_JMP_NO_DATA) * 65536 + COMPDARK(1,536,PIO_JMP_NO_DATA);
uint32_t hd_line_sync_bl[2] = {
  COMPDARK(1, 8,PIO_JMP_NO_DATA) * 65536 + COMPDARK(0,19,PIO_JMP_NO_DATA),
  COMPDARK(0,477,PIO_JMP_NO_DATA) * 65536 + COMPDARK(0,34,PIO_JMP_NO_DATA),
};
/* Original front / back porch
uint32_t hd_line_sync_dt[2] = {
  COMPDARK(1, 8,PIO_JMP_NO_DATA) * 65536 + COMPDARK(0,19,PIO_JMP_NO_DATA),
  COMPDARK(0,476,PIO_JMP_START_DATA) * 65536 + COMPDARK(0,34,PIO_JMP_NO_DATA),
  }; */

/* Adjusted to center image (60 dark pixels before and after) */
uint32_t hd_line_sync_dt[2] = {
  COMPDARK(1, 8,PIO_JMP_NO_DATA) * 65536 + COMPDARK(0,67,PIO_JMP_NO_DATA),
  COMPDARK(0,380,PIO_JMP_START_DATA) * 65536 + COMPDARK(0,82,PIO_JMP_NO_DATA),
  };


displist interlace[14] =
  {{&vert_sync, 0, 5},
   {&equalize_sync, 0, 5}, //Even frame
   {&equalize_emt, 0, 1},
   {line_sync_bl, 1, 38},
   {line_sync_dt, 1, 256},
   {line_sync_bl, 1, 9},
   {&equalize_sync, 0, 6},

   {&vert_sync, 0, 5},
   {&equalize_sync, 0, 6}, //Odd frame
   {&equalize_emt, 0, 1},
   {line_sync_bl, 1, 38},
   {line_sync_dt, 1, 256},
   {line_sync_bl, 1, 10},
   {&equalize_sync, 0, 5}};

displist non_interlace[7] =
  {{&vert_sync, 0, 5},
   {&equalize_sync, 0, 6}, //Even frame
   {&equalize_emt, 0, 1},
   {line_sync_bl, 1, 38},
   {line_sync_dt, 1, 256},
   {line_sync_bl, 1, 9},
   {&equalize_sync, 0, 6}};


// VGA Signal 640 x 480 @ 60 Hz Industry standard timing
// http://www.tinyvga.com/vga-timing/640x480@60Hz
displist vga_standard [4] =
  {{vga_line_sync_bl, 1, 11},
   {&vga_vert_sync, 0, 2},
   {vga_line_sync_bl, 1, 31},
   {vga_line_sync_dt, 1, 480}
  };
//522 * 2 + 2 = 1044+2 = 1046


/* Original HD standard
displist hd_standard [4] =
  {{hd_line_sync_bl, 1, 4},
   {&hd_vert_sync, 0, 5},
   {hd_line_sync_bl, 1, 36},
   {hd_line_sync_dt, 1, 1080}
  };
  // 8 + 72 + 2160 + 5 = 2245 */

// Adjusted to come to an effective display region of 512 * 768 (each line is tripled)
/*
displist hd_standard [4] =
  {{&hd_vert_sync, 0, 5},
   {hd_line_sync_bl, 1, 64},
   {hd_line_sync_dt, 1, 1024},
   {hd_line_sync_bl, 1, 32}
  };
*/
displist hd_standard [4] =
  {{&hd_vert_sync, 0, 5},
   {hd_line_sync_bl, 1, 56},
   {hd_line_sync_dt, 1, 1024},
   {hd_line_sync_bl, 1, 40}
  };


// Wenn fmat -> odd_frame = 1 und 608 zeilen

// Precalculate display list
// 12 + 76 + 512 + 18 + 6 =  624 * 4 = 1248 = 2496B
//uint32_t fulldlist[624];
//uint32_t fulldlist[1046];
uint32_t fulldlist[4096];

// Graphics memory (i.e. the content)
uint32_t graphmem_4p[16384];

uint32_t *graphmem;        // This points to the page that is shown
uint32_t *graphmem_write;  // This points to the page where the drawing happens

uint32_t line_count;
// Pixels in line (Actually 512 pixels. However, x register must be loaded with 1 less for the first round
uint32_t line_len = 511;


//
// Buffers for color translation
//
uint32_t line_buf0[128];
uint32_t line_buf1[128];


/****************************************************************************
 * Name: calc_fulldlist
 *
 * Description:
 *   Creates the full displaylist as a sequence of 16 bit chunks
 *   that control the PIO to generate a proper SYNC signal for a complete
 *   frame.
 *
 * Input Parameters:
 *   displist   - A set of instructions that describe the construction
 *                of a displaylist to create a certain SYNC pattern.
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

void calc_fulldlist (displist *dlist)
{
  uint32_t lpat_count = 0;
  uint32_t lcount = 0;
  uint32_t *line_sync = dlist[lpat_count].lsync;
  uint32_t hlcount = 0;
  int bfin = 0;
  int indx = 0;

  // Initialize "displaylist"
  while (!bfin)
    {
      fulldlist[indx++] = line_sync[hlcount];
      ++hlcount;

      if (hlcount > dlist[lpat_count].len)
	{
	  hlcount = 0;
	  ++lcount;
	  if (lcount >= dlist[lpat_count].count)
	    {
	      lcount = 0;
	      ++lpat_count;
	      //	      if (lpat_count > 6)
	      if (lpat_count > 3)
		{
		  lpat_count = 0;
		  bfin = 1;
		}
	      line_sync = dlist[lpat_count].lsync;
	    }
	}
    }

  line_count = 0;
}


// Since the io register bank cannot be accessed from interrupt
// handlers to avoid deadlocks from the used spinlock, a
// separate variable is switched when the dma interrupt, indicating
// a finished frame, is called.
uint8_t vsync_flag;


/****************************************************************************
 * Name: gdp_sync_dma_handler
 *
 * Description:
 *   Interrupt routine to handle the DMA interrupt when the
 *   displaylist has been completed.
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

// Since the full "displaylist" is precalculated, the
// DMA handler is quite simple in structure
void __not_in_flash_func(gdp_sync_dma_handler) (void)
{
  dma_channel_set_read_addr(dma_channel_0, fulldlist, true);

  // Somehow, the synchronization is not perfect. Hence, reset the line
  // counter always at the beginning of a new sync pattern
  line_count = 0;

  // Set flag for vertical blank interrupt
  vsync_flag = 0x1;

  // Finally, clear the interrupt request ready for the next horizontal
  // sync interrupt
  dma_hw->ints0 = 1u << dma_channel_0;
}

/****************************************************************************
 * Name: plot_pixel
 *
 * Description:
 *   Draws/Erases a pixel in the active framebuffer. Whether a pixel
 *   is drawn at all and whether an erase or draw operation should
 *   carried out is determined from the GDP registers.
 *
 * Input Parameters:
 *   x      - x coordinate
 *   y      - y coordinate
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

void plot_pixel (int x, int y)
{
  // Check if pixel is in (visible) screen
  if ((x >= 0) && (x < 512) && (y >= 0) && (y < 256))
    {
      int mpos = ((255-(y & 0xFF)) * 16 + (x >> 5)) & (4096 - 1);
      int bpos = 31 - (x & 0x1F);
      uint8_t ctrl1 = read_io_reg (gdp_ctrl1);

      // Check control register 1
      if ((ctrl1 & 0x3) == 0x3)
	graphmem_write[mpos] = graphmem_write[mpos] | 1u << bpos;
      if ((ctrl1 & 0x3) == 0x1)
	graphmem_write[mpos] = graphmem_write[mpos] & ~(1u << bpos);
    }
}

/****************************************************************************
 * Name: gdp_proc_command
 *
 * Description:
 *   The EF9365 graphics commands are initiated by a write to the
 *   command registers. This functions receives such a command
 *   value and carries the respective function out.
 *
 * Input Parameters:
 *   gdp_cmd   - Command as described in the datasheet for the EF9365
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

void gdp_proc_command (unsigned char gdp_cmd)
{
  /* Debug code to trace GDP commands
  unsigned int x_ref = read_io_reg(gdp_xmsb) * 256 + read_io_reg(gdp_xlsb),
    y_ref = read_io_reg(gdp_ymsb) * 256 + read_io_reg(gdp_ylsb);
  uint8_t ctrl1 = read_io_reg (gdp_ctrl1);
  printf ("G %02x  X%04x   Y%04x  CT1%02x\n", gdp_cmd, x_ref, y_ref, ctrl1);
  */

  if (gdp_cmd < 0x10)
    {
      switch (gdp_cmd)
	{
	  // Set Bit 1 of Ctrl 1
	case 0x0:
	  change_io_reg (gdp_ctrl1, 0x2, 0);
	  break;
	  // Clear Bit 1 of Ctrl 1
	case 0x1:
	  change_io_reg (gdp_ctrl1, 0, 0x2);
	  break;
	  // Set Bit 0 of Ctrl 1
	case 0x2:
	  change_io_reg (gdp_ctrl1, 0x1, 0);
	  break;
	  // Clear Bit 0 of Ctrl 1
	case 0x3:
	  change_io_reg (gdp_ctrl1, 0, 0x1);
	  break;
	case 0x7:
	  write_io_reg (gdp_ctrl1, 0);
	  write_io_reg (gdp_ctrl2, 0);
	  write_io_reg (gdp_csize, 0x11);
	case 0x6:
	  write_io_reg (gdp_xmsb, 0);
	  write_io_reg (gdp_xlsb, 0);
	  write_io_reg (gdp_ymsb, 0);
	  write_io_reg (gdp_ylsb, 0);
	case 0x4:
	  memset (graphmem_write, 0, 16384);
	  break;
	case 0x5:
	  write_io_reg (gdp_xmsb, 0);
	  write_io_reg (gdp_xlsb, 0);
	  write_io_reg (gdp_ymsb, 0);
	  write_io_reg (gdp_ylsb, 0);
	  break;
	case 0xA:
	  draw_char (128u); // 0x20 + 96
	  break;
	}
    }
  else
    {
      if (gdp_cmd >= 0x20)
	{
	  if (gdp_cmd < 0x80)
	    draw_char (gdp_cmd);
	  else
	    draw_line (gdp_cmd);
	}
      else
	draw_line (gdp_cmd);
    }
}

/****************************************************************************
 * Name: gdp_set_pages
 *
 * Description:
 *   As a custom function of the GDP64 it was possible to select
 *   one of 4 pages in the 64k graphics memory. This functionality
 *   is implemented in thsi function.
 *
 * Input Parameters:
 *   r_page   - Page to be displayed (Range 0..3)
 *   w_page   - Page on which graphics commands operate (Range 0..3)
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

void gdp_set_pages (unsigned int r_page, unsigned int w_page)
{
  graphmem = &(graphmem_4p[4096*r_page]);
  graphmem_write = &(graphmem_4p[4096*w_page]);
}

// The address into the lookup table is generated
// by using an or operation between the bit pattern
// and the base address. Hence, the look up table
// must be properly aligned.
uint8_t __attribute__((aligned(8))) gdp_lut[16];

uint dma_lut_channel_0; // DMA channel for transferring data to PIO
uint dma_lut_channel_1; // DMA channel for transferring LUT addresses from PIO
uint dma_lut_channel_2; // DMA channel for reading colors from LUT

dma_channel_config dma_cconf_0;
dma_channel_config dma_cconf_2;


/****************************************************************************
 * Name: gdp_do_lut_map
 *
 * Description:
 *   Configures dma channels to do a lut mapping. After configuration
 *   the dma channels are started to execute the LUT mapping.
 *
 * Input Parameters:
 *   src    - Input data which is represented as a sequence of
 *            bit patterns. For 1 bit color depth:
 *            Pix0, Pix1, Pix2, etc.
 *            For 4 bit color depth:
 *            Pix0_c3, Pix0_c2, Pix0_c1, Pix0_c0, Pix1_c3, Pix1_c2, etc.
 *   src_len- Number of 32 bit words in the input data
 *   dst    - Output data. Sequence of 8 bit color maps (according to
 *            resistors and color mapping of the GPIOs).
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

void gdp_do_lut_map (uint32_t *src, uint32_t src_len, uint32_t *dst)
{
  PIO pio = pio1;

  dma_channel_configure(dma_lut_channel_0, &dma_cconf_0,
			&pio->txf[sm_gdp_lut], // Destination pointer
			src,                   // Source pointer
			src_len,               // Set to proper line length
			false);                // Do not start yet

  dma_channel_configure(dma_lut_channel_2, &dma_cconf_2,
			dst,                   // Destination pointer
			NULL,                  // Filled in by DMA channel 1
			1,                     // Halt after each transfer
			false);                // Do not start yet

  // Start the initial DMA channel, which waits for a RDrequest from the PIO
  dma_channel_start (dma_lut_channel_0);
}

/****************************************************************************
 * Name: gdp_init_lut_map
 *
 * Description:
 *   Pre-Initializes the 3 DMA channels for the DMA/PIO driven LUT
 *   mapping.
 *
 * Input Parameters:
 *   pio    - PIO in which the LUT statemachine resides
 *   sm     - SM which implements the LUT mapping support
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

void gdp_init_lut_map (PIO pio, uint sm)
{
  // Some test colors
  /* Gives a blue background with white color. Typical homecomputer style
  gdp_lut[0] = 0x12;
  gdp_lut[1] = 0xFF;
  */
  // Amber look (R=FF G=BF B =00)
  gdp_lut[0] = 0x0;
  /*
  gdp_lut[1] = (0 << 5) + (6 << 3) + (7); // Yields some blueish color
  gdp_lut[1] = 7 << 2;  // Classic Green
  gdp_lut[1] = 1 << 2;  // Very faint Green
  gdp_lut[1] = (0) + (6 << 2) + (7 << 5);  // B G R  -> Lighter amber
  */
  gdp_lut[1] = (0) + (5 << 2) + (7 << 5);  // B G R  -> Darker amber

  // DMA Channel 0 -> Read screen data and put it into PIO
  // DMA Channel 1 -> Read addresses from PIO and put those addresses into DMA channel 2
  // DMA Channel 2 -> Read from PIO address and write to output data
  dma_lut_channel_0 = dma_claim_unused_channel(true);
  dma_lut_channel_1 = dma_claim_unused_channel(true);
  dma_lut_channel_2 = dma_claim_unused_channel(true);

  // This DMA channel has one function. Take the screen data and
  // put it the LUT PIO
  dma_cconf_0 = dma_channel_get_default_config(dma_lut_channel_0);
  channel_config_set_transfer_data_size(&dma_cconf_0, DMA_SIZE_32);
  channel_config_set_read_increment(&dma_cconf_0, true);
  channel_config_set_write_increment(&dma_cconf_0, false);
  channel_config_set_dreq(&dma_cconf_0, pio_get_dreq(pio, sm, true));

  // Receive address from the PIO, start DMA channel 2
  dma_channel_config c = dma_channel_get_default_config(dma_lut_channel_1);
  channel_config_set_transfer_data_size(&c, DMA_SIZE_32);
  channel_config_set_read_increment(&c, false);
  channel_config_set_write_increment(&c, false);
  channel_config_set_dreq(&c, pio_get_dreq(pio, sm, false));   // RX

  dma_channel_configure(dma_lut_channel_1, &c,
			&dma_hw->ch[dma_lut_channel_2].al3_read_addr_trig,    // Destination pointer
			&pio->rxf[sm],    // Source pointer (PIO output)
			1,                // Halt after each transfer
			true);            // Start immediately

  // This channel is configures by channel 1 and transfers one byte
  // from the address delivered by the PIO to the output array
  dma_cconf_2 = dma_channel_get_default_config(dma_lut_channel_2);
  channel_config_set_transfer_data_size(&dma_cconf_2, DMA_SIZE_8);
  channel_config_set_read_increment(&dma_cconf_2, false);
  channel_config_set_write_increment(&dma_cconf_2, true);
  channel_config_set_chain_to(&dma_cconf_2, dma_lut_channel_1);
  channel_config_set_dreq(&dma_cconf_2, 0x3F);  // Unpace transfer

  // Start PIO and transfer address of LUT into PIO
  pio_sm_set_enabled(pio, sm_gdp_lut, true);
  pio_sm_put_blocking (pio, sm_gdp_lut, (uint32_t)(&gdp_lut) >> 1);
}

/****************************************************************************
 * Name: gdp_data_dma_handler
 *
 * Description:
 *   Interrupt routine for the data DMA channel for the PIO that outputs
 *   the pixel/picture data.
 *   In vertical direction each line is displayed 4 times to expand the 256
 *   lines to the 1080 lines in the 1920x1080P format.
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

// DMA handler for data provision to PIO
void __not_in_flash_func(gdp_data_dma_handler)(void)
{
  // Set graphics output to respective pointer in graphics memory
  // (Note: Each line is repeated 4 times)
  ++line_count;
  if (line_count > 1023)
    line_count = 0;

  if (line_count & 0x4)
    dma_channel_set_read_addr(dma_channel_1, (uint32_t *)&line_buf0, true);
  else
    dma_channel_set_read_addr(dma_channel_1, (uint32_t *)&line_buf1, true);

  // Fill buffer that is currently not in use with new color data
  if ((line_count & 0x3) == 0)   // line_count % 4 == 0
    {
      uint32_t *src, *dst;
      PIO pio = pio1;

      src = &graphmem[(((line_count >> 2) + 1) & 0xFF) << 4];
      if (line_count & 0x4)
	dst = line_buf1;
      else
	dst = line_buf0;

      dma_channel_configure(dma_lut_channel_0, &dma_cconf_0,
			    &pio->txf[sm_gdp_lut], // Destination pointer
			    src,                   // Source pointer
			    16,                    // Set to proper line length
			    false);                // Do not start yet

      dma_channel_configure(dma_lut_channel_2, &dma_cconf_2,
			    dst,                   // Destination pointer
			    NULL,                  // Filled in by DMA channel 1
			    1,                     // Halt after each transfer
			    false);                // Do not start yet

      // Start the initial DMA channel, which waits for a RDrequest from the PIO
      dma_channel_start (dma_lut_channel_0);
    }

  // Clear VB flag if it is still set
  vsync_flag = 0;

  // Finally, clear the interrupt request ready for the next horizontal sync interrupt
  dma_hw->ints1 = 1u << dma_channel_1;
}


// Queue and task for receiving gdp commands
#define GDP_QUEUE_LENGTH 4
StaticQueue_t gdp_queue_buf;
QueueHandle_t gdp_queue;
uint8_t ucGDPQueueStorage[GDP_QUEUE_LENGTH * PBUS_QUEUE_IS];
TaskHandle_t gdp_task;

/****************************************************************************
 * Name: gdp_proc_monitor
 *
 * Description:
 *   FreeRTOS task to listen for commands from the parallel bus.
 *   Picks the request up, executes the command and then sets the
 *   GDP ready flag.
 *
 * Input Parameters:
 *   unused_arg   - Not used.
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

void gdp_proc_monitor(void* unused_arg) {
  uint32_t fifo_cmd;
  uint8_t reg, data;

  while (1)
    {
      if (xQueueReceive (gdp_queue, &fifo_cmd, (TickType_t) 10))
	{
	  reg = (fifo_cmd >> 8) & 0xFF;
	  if (reg == 0x70)
	    {
	      gdp_proc_command (fifo_cmd & 0xFF);
	      change_io_reg (0x70, 0x4, 0); // For the GDP high indicates "not busy"
	    }
	}
    }
}


StaticQueue_t gdp_page_queue_buf;
QueueHandle_t gdp_page_queue;
uint8_t ucGDPPageQueueStorage[GDP_QUEUE_LENGTH * PBUS_QUEUE_IS];
TaskHandle_t gdp_page_task;

/****************************************************************************
 * Name: gdp_page_monitor
 *
 * Description:
 *   Task to monitor changes to the page register as communicated
 *   from the parallel bus through the respective queue.
 *   Furthermore this task updates the Z80 IO register according
 *   to the vsync_flag.
 *
 * Input Parameters:
 *   unused_arg   - Not used.
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

void gdp_page_monitor(void* unused_arg) {
  uint32_t fifo_cmd;
  uint8_t reg, data;
  uint8_t old_vsync = vsync_flag;

  while (1)
    {
      // Core function
      if (xQueueReceive (gdp_page_queue, &fifo_cmd, (TickType_t) 1))
	{
	  reg = (fifo_cmd >> 8) & 0xFF;
	  if (reg == 0x60)
	    {
	      data = fifo_cmd & 0xFF;
	      gdp_set_pages ((data >> 4) & 0x3, (data >> 6) & 0x3);
	    }
	}

      // Side function, copy the sync flag to GDP register
      if (vsync_flag != old_vsync)
	{
	  if (vsync_flag)
	    change_io_reg (0x70, 0x2, 0);
	  else
	    change_io_reg (0x70, 0, 0x2);
	  old_vsync = vsync_flag;
	}
    }
}

/****************************************************************************
 * Name: init_gdp
 *
 * Description:
 *   Initializes the GDP functions. I.e. creation of the displaylist,
 *   setup of the DMA channels for SYNC and DATA, setup of the PIO
 *   for graphics data.
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

// Initialize the GDP "simulation"
int init_gdp ()
{
  /*
   * Set memory to page 0 and initialize
   * the DMA control block that points to the graphics memory
   */
  graphmem = (uint32_t *)&graphmem_4p;
  graphmem_write = graphmem;

  /*
   * TODO: Actually, the code should also work with pio0! However, it
   * does not, which points to an error in some other part of the code#
   */

  PIO pio = pio1;
  uint offset_sync = pio_add_program(pio, &gdp_sync_program);
  uint offset_data = pio_add_program(pio, &gdp_data_program);
  uint offset_lut = pio_add_program(pio, &gdp_lut_program);

  dma_channel_0 = dma_claim_unused_channel(true);	// Claim a DMA channel for the sync
  dma_channel_1 = dma_claim_unused_channel(true);	// Data channel

  // Initial test step for DMA
  calc_fulldlist (hd_standard);

  pio_sm_set_enabled(pio, sm_gdp_sync, false);
  pio_sm_clear_fifos(pio, sm_gdp_sync);

  pio_sm_set_enabled(pio, sm_gdp_data, false);
  pio_sm_clear_fifos(pio, sm_gdp_data);

  pio_sm_set_enabled(pio, sm_gdp_lut, false);
  pio_sm_clear_fifos(pio, sm_gdp_lut);

  // DMA for SYNC signal
  dma_channel_config c = dma_channel_get_default_config(dma_channel_0);
  channel_config_set_transfer_data_size(&c, DMA_SIZE_32);
  channel_config_set_read_increment(&c, true);
  channel_config_set_dreq(&c, pio_get_dreq(pio, sm_gdp_sync, true));

  dma_channel_configure(dma_channel_0, &c,
        &pio->txf[sm_gdp_sync],      // Destination pointer
        (void *) fulldlist,          // Source pointer
        2245,                        // Size of buffer
        false                        // Start flag (true = start immediately)
    );

  dma_channel_set_irq0_enabled(dma_channel_0, true);
  irq_set_exclusive_handler(DMA_IRQ_0, gdp_sync_dma_handler);
  irq_set_enabled(DMA_IRQ_0, true);

  // DMA channel for the video data
  c = dma_channel_get_default_config(dma_channel_1);
  channel_config_set_transfer_data_size(&c, DMA_SIZE_32);
  channel_config_set_read_increment(&c, true);
  channel_config_set_write_increment(&c, false);
  channel_config_set_dreq(&c, pio_get_dreq(pio, sm_gdp_data, true));

  dma_channel_configure(dma_channel_1, &c,
			&pio->txf[sm_gdp_data],        // Destination pointer
			&graphmem[0],                       // Source pointer
			128,                // Size of buffer
			false);

  dma_channel_set_irq1_enabled(dma_channel_1, true);
  irq_set_exclusive_handler(DMA_IRQ_1, gdp_data_dma_handler);
  irq_set_enabled(DMA_IRQ_1, true);

  gdp_program_init(pio, sm_gdp_sync, sm_gdp_data, sm_gdp_lut,
		   offset_sync, offset_data, offset_lut);

  // Set line length for data PIO (will be cached in X register of the PIO)
  pio_sm_put_blocking (pio, sm_gdp_data, line_len);

  // Start PIO for SYNC signal
  pio_sm_set_enabled(pio, sm_gdp_sync, true);

  // Initialize channels for color LUT
  gdp_init_lut_map (pio, sm_gdp_lut);

  /* Start display DMA */
  dma_channel_start (dma_channel_0);
  dma_channel_start (dma_channel_1);

  // Initialize queue and task for processing GDP commands
  gdp_queue = xQueueGenericCreateStatic(GDP_QUEUE_LENGTH,
					PBUS_QUEUE_IS,
					&(ucGDPQueueStorage[0]),
					&gdp_queue_buf,
					queueQUEUE_TYPE_BASE);

  BaseType_t monitor_status = xTaskCreate(gdp_proc_monitor,
					  "GDP_TASK",
					  512,
					  NULL,
					  1,
					  &gdp_task);

  // Initialize queue to manage page switches
  gdp_page_queue = xQueueGenericCreateStatic(GDP_QUEUE_LENGTH,
					PBUS_QUEUE_IS,
					&(ucGDPPageQueueStorage[0]),
					&gdp_page_queue_buf,
					queueQUEUE_TYPE_BASE);

  monitor_status = xTaskCreate(gdp_page_monitor,
			       "GDP_PAGE_TASK",
			       512,
			       NULL,
			       1,
			       &gdp_page_task);

  return (0);
}
