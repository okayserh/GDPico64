/*
 * xmodem_pico.c
 *
 * Copyright 2001-2022 Georges Menie (www.menie.org)
 * https://www.menie.org/georges/embedded/xmodem.html
 *
 * Copyright 2024 Oliver Kayser-Herold (Adaptations to use in project)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * Provides callbacks for xmodem receiver
 */

#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/pio.h"
#include <stdio.h>
#include <string.h>

#include "stdio_dev.h"

#define SOH  0x01
#define STX  0x02
#define EOT  0x04
#define ACK  0x06
#define NAK  0x15
#define CAN  0x18
#define CTRLZ 0x1A

#define DLY_1S 1000
#define MAXRETRANS 25

// Buffer
uint8_t xmod_buffer[4096];
uint32_t xmod_len = 0;

// Originally in xmodemReceive function. Since stack
// size is limited, put into global variable space
static unsigned char xbuff[1030]; /* 1024 for XModem 1k + 3 head chars + 2 crc + nul */


/****************************************************************************
 * Name: crc_xmodem_update
 *
 * Description:
 *   Update CRC calculation with the latest received data.
 *
 * Input Parameters:
 *   crc   - Current CRC16
 *   data  - Newly received data
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

uint16_t crc_xmodem_update (uint16_t crc, uint8_t data)
{
    int i;
    crc = crc ^ ((uint16_t)data << 8);
    for (i=0; i<8; i++)
    {
        if (crc & 0x8000)
            crc = (crc << 1) ^ 0x1021;
        else
            crc <<= 1;
    }
    return crc;
}

/****************************************************************************
 * Name: crc16_ccitt
 *
 * Description:
 *   Calculates the CRC16 for a buffer of given size. (Uses crc_xmodem_udpate
 *   for the core calculation)
 *
 * Input Parameters:
 *   buf    - Buffer with data
 *   sz     - Number of bytes in the buffer that should be included
 *            in the CRC calculation.
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

uint16_t crc16_ccitt (const uint8_t *buf, int sz)
{
  uint16_t crc = 0;
  int i = 0;

  for (i = 0; i < sz; ++i)
    crc = crc_xmodem_update (crc, buf[i]);

  return (crc);
}


/****************************************************************************
 * Name: check
 *
 * Description:
 *   Calculate the CRC16 for a buffer and verify that it matches
 *   the transmitted CRC16.
 *
 * Input Parameters:
 *   crc    - Flag indicating wheter CRC16 (crc != 0) or
 *            of a simple checksum should be used to verify
 *            the correctness of the received data.
 *   buf    - Buffer with data to be verified. (First two bytes
 *            include CRC16 when CRC16 is used, otherwise the
 *            first byte includes the chksum)
 *   sz     - Number of bytes over which the checksum should
 *            be calculated.
 *
 * Returned Value:
 *   1   Checksum/CRC16 is OK.
 *   0   Checksum/CRC16 does not match
 *
 ****************************************************************************/

static int check(int crc, const uint8_t *buf, int sz)
{
  if (crc) {
    uint16_t crc = crc16_ccitt(buf, sz);
    uint16_t tcrc = (buf[sz]<<8)+buf[sz+1];
    if (crc == tcrc)
      return 1;
  }
  else {
    int i;
    uint8_t cks = 0;
    for (i = 0; i < sz; ++i) {
      cks += buf[i];
    }
    if (cks == buf[sz])
      return 1;
  }

  return 0;
}

/****************************************************************************
 * Name: _inbyte
 *
 * Description:
 *   Receive one byte from serial interface with timeout
 *
 * Input Parameters:
 *   timeout_ms - Time to wait for character in ms.
 *
 * Returned Value:
 *   -1  Error or Timeout
 *   The received character
 *
 ****************************************************************************/

static int _inbyte (const uint32_t timeout_ms)
{
  uint8_t ch;
  if (xQueueReceive(stdio_dev.input_queue, &ch, pdMS_TO_TICKS (timeout_ms)))
    return (ch);
  else
    return (-1);
}

/****************************************************************************
 * Name: _outbyte
 *
 * Description:
 *   Write one byte to the serial port
 *
 * Input Parameters:
 *   ch     - Character to be output to the serial port
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static void _outbyte (const uint8_t ch)
{
  BaseType_t xHigherPriorityTaskWoken;
  //  xQueueSend (stdio_dev.output_queue, &ch, &xHigherPriorityTaskWoken);
  putchar_raw (ch);
}

/****************************************************************************
 * Name: flushinput
 *
 * Description:
 *   Try to read bytes from serial port until a 1s timeout happens.
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static void flushinput(void)
{
  while (_inbyte(((DLY_1S)*3)>>1) >= 0)
    ;
}

/****************************************************************************
 * Name: xmodem_receive
 *
 * Description:
 *   Receive a file through the serial port using the XModem protocol.
 *
 * Input Parameters:
 *   dest   - Destination address for received data
 *   destsz - Max size of destination buffer
 *
 * Returned Value:
 *   <0   Error code
 *   >0   Number of received bytes
 *
 ****************************************************************************/

// https://www.menie.org/georges/embedded/xmodem.html

int xmodemReceive(unsigned char *dest, int destsz)
{
  unsigned char *p;
  int bufsz, crc = 0;
  unsigned char trychar = 'C';
  unsigned char packetno = 1;
  int i, c, len = 0;
  int retry, retrans = MAXRETRANS;

  for(;;) {
    for( retry = 0; retry < 16; ++retry) {
      if (trychar) _outbyte(trychar);
      if ((c = _inbyte((DLY_1S)<<1)) >= 0) {
	switch (c) {
	case SOH:
	  bufsz = 128;
	  goto start_recv;
	case STX:
	  bufsz = 1024;
	  goto start_recv;
	case EOT:
	  flushinput();
	  _outbyte(ACK);
	  return len; /* normal end */
	case CAN:
	  if ((c = _inbyte(DLY_1S)) == CAN) {
	    flushinput();
	    _outbyte(ACK);
	    return -1; /* canceled by remote */
	  }
	  break;
	default:
	  break;
	}
      }
    }
    if (trychar == 'C') { trychar = NAK; continue; }
    flushinput();
    _outbyte(CAN);
    _outbyte(CAN);
    _outbyte(CAN);
    return -2; /* sync error */

  start_recv:
    if (trychar == 'C') crc = 1;
    trychar = 0;
    p = xbuff;
    *p++ = c;
    for (i = 0;  i < (bufsz+(crc?1:0)+3); ++i) {
      if ((c = _inbyte(DLY_1S)) < 0) goto reject;
      *p++ = c;
    }

    if (xbuff[1] == (unsigned char)(~xbuff[2]) &&
	(xbuff[1] == packetno || xbuff[1] == (unsigned char)packetno-1) &&
	check(crc, &xbuff[3], bufsz)) {
      if (xbuff[1] == packetno)	{
	register int count = destsz - len;
	if (count > bufsz) count = bufsz;
	if (count > 0) {
	  memcpy (&dest[len], &xbuff[3], count);
	  len += count;
	}
	++packetno;
	retrans = MAXRETRANS+1;
      }
      if (--retrans <= 0) {
	flushinput();
	_outbyte(CAN);
	_outbyte(CAN);
	_outbyte(CAN);
	return -3; /* too many retry error */
      }
      _outbyte(ACK);
      continue;
    }
  reject:
    flushinput();
    _outbyte(NAK);
  }
}

/****************************************************************************
 * Name: xmodemTransmit
 *
 * Description:
 *   Send a file through the serial port using the XModem protocol.
 *
 * Input Parameters:
 *   src    - Source address for data to be sent
 *   srcsz  - Size of data to be transmitted
 *
 * Returned Value:
 *   <0   Error code
 *   >0   Number of sent bytes
 *
 ****************************************************************************/

int xmodemTransmit(unsigned char *src, int srcsz)
{
  int bufsz, crc = -1;
  unsigned char packetno = 1;
  int i, c, len = 0;
  int retry;

  for(;;) {
    for( retry = 0; retry < 16; ++retry) {
      if ((c = _inbyte((DLY_1S)<<1)) >= 0) {
	switch (c) {
	case 'C':
	  crc = 1;
	  goto start_trans;
	case NAK:
	  crc = 0;
	  goto start_trans;
	case CAN:
	  if ((c = _inbyte(DLY_1S)) == CAN) {
	    _outbyte(ACK);
	    flushinput();
	    return -1; /* canceled by remote */
	  }
	  break;
	default:
	  break;
	}
      }
    }
    _outbyte(CAN);
    _outbyte(CAN);
    _outbyte(CAN);
    flushinput();
    return -2; /* no sync */
    
    for(;;) {
    start_trans:
      xbuff[0] = SOH; bufsz = 128;
      xbuff[1] = packetno;
      xbuff[2] = ~packetno;
      c = srcsz - len;
      if (c > bufsz) c = bufsz;
      if (c >= 0) {
	memset (&xbuff[3], 0, bufsz);
	if (c == 0) {
	  xbuff[3] = CTRLZ;
	}
	else {
	  memcpy (&xbuff[3], &src[len], c);
	  if (c < bufsz) xbuff[3+c] = CTRLZ;
	}
	if (crc) {
	  unsigned short ccrc = crc16_ccitt(&xbuff[3], bufsz);
	  xbuff[bufsz+3] = (ccrc>>8) & 0xFF;
	  xbuff[bufsz+4] = ccrc & 0xFF;
	}
	else {
	  unsigned char ccks = 0;
	  for (i = 3; i < bufsz+3; ++i) {
	    ccks += xbuff[i];
	  }
	  xbuff[bufsz+3] = ccks;
	}
	for (retry = 0; retry < MAXRETRANS; ++retry) {
	  for (i = 0; i < bufsz+4+(crc?1:0); ++i) {
	    _outbyte(xbuff[i]);
	  }
	  if ((c = _inbyte(DLY_1S)) >= 0 ) {
	    switch (c) {
	    case ACK:
	      ++packetno;
	      len += bufsz;
	      goto start_trans;
	    case CAN:
	      if ((c = _inbyte(DLY_1S)) == CAN) {
		_outbyte(ACK);
		flushinput();
		return -1; /* canceled by remote */
	      }
	      break;
	    case NAK:
	    default:
	      break;
	    }
	  }
	}
	_outbyte(CAN);
	_outbyte(CAN);
	_outbyte(CAN);
	flushinput();
	return -4; /* xmit error */
      }
      else {
	for (retry = 0; retry < 10; ++retry) {
	  _outbyte(EOT);
	  if ((c = _inbyte((DLY_1S)<<1)) == ACK) break;
	}
	flushinput();
	return (c == ACK)?len:-5;
      }
    }
  }
}

/****************************************************************************
 * Name: xmodem_receive
 *
 * Description:
 *   Wrapper to call xmodemReceive with the internal xmodem buffer. Used
 *   to provide a means to transfer programs from the Host computer
 *   to the Pico/Z80 environment.
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

int xmodem_receive ()
{
  int xm_rc = 0;

  for (unsigned int i = 0; i < sizeof(xmod_buffer); ++i)
    xmod_buffer[i] = 0;
  xmod_len = 0;

  xm_rc = xmodemReceive (xmod_buffer, 4096);
  if (xm_rc < 0)
    printf ("XModem Transfer Error!\n");
  else
    {
      printf ("XModem Received %i\n", xm_rc);
      xmod_len = xm_rc;
    }
  return (0);
}

/****************************************************************************
 * Name: xmodem_send
 *
 * Description:
 *   Wrapper to call xmodemTransmit with the internal xmodem buffer. Used
 *   to provide a means to transfer programs from the Pico/Z80 environment
 *   to Host computer.
 *
 * Input Parameters:
 *   parm   - As provided to the initiatlization of the callback. Should
 *            be the respective FreeRTOS Queue
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

int xmodem_send ()
{
  int xm_rc = 0;

  xm_rc = xmodemTransmit (xmod_buffer, xmod_len);
  if (xm_rc < 0)
    printf ("XModem Transfer Error!\n");
  else
    {
      printf ("XModem Transmitted %i\n", xm_rc);
    }
  return (0);
}


/****************************************************************************
 * Name: dump_xmod_buffer
 *
 * Description:
 *   Function to output the xmodem buffer to the console for test
 *   purposes.
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

int dump_xmod_buffer ()
{
  for (unsigned int i = 0; i < xmod_len; ++i)
    {
      printf ("%02x ", xmod_buffer[i]);
      if (i % 8 == 7)
	printf ("\n");
    }
  printf ("\n");
  return (0);
}
