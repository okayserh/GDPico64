/**
 * par_bus.h
 *
 * Implementation of the parallel IO bus interface to the Z80
 * and potentially other devices.
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

#ifndef _NDRNKC_PAR_BUS_
#define _NDRNKC_PAR_BUS__

#include "hardware/sync.h"
#include "pico/stdlib.h"

#define IO_SPIN_LOCK_NUM  2

// For FreeRTOS Queue that receives results
// from parallel bus via RP2040 interprocessor queue
#define PBUS_QUEUE_IS 4

extern uint8_t z80_mem[256];


__force_inline static uint8_t read_io_reg (uint8_t adr)
{
  uint32_t save_irq;
  uint8_t res = 0;
  spin_lock_t *io_spin_lock = spin_lock_instance (IO_SPIN_LOCK_NUM);
  save_irq = spin_lock_blocking (io_spin_lock);
  res = z80_mem[adr];
  spin_unlock (io_spin_lock, save_irq);
  return (res);
}

__force_inline static void write_io_reg (uint8_t adr, uint8_t data)
{
  uint32_t save_irq;
  spin_lock_t *io_spin_lock = spin_lock_instance (IO_SPIN_LOCK_NUM);
  save_irq = spin_lock_blocking (io_spin_lock);
  z80_mem[adr] = data;
  spin_unlock (io_spin_lock, save_irq);
}


__force_inline static void change_io_reg (uint8_t adr, uint8_t set_bit, uint8_t clear_bit)
{
  uint32_t save_irq;
  spin_lock_t *io_spin_lock = spin_lock_instance (IO_SPIN_LOCK_NUM);
  save_irq = spin_lock_blocking (io_spin_lock);
  z80_mem[adr] |= set_bit;
  z80_mem[adr] &= ~clear_bit;
  spin_unlock (io_spin_lock, save_irq);
}


/*
 * Starts the process to process activity
 * on the parallel bus on core1
 */

int init_par_bus ();

#endif
