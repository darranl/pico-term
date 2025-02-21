/* Copyright 2024, Darran A Lofthouse
 *
 * This file is part of pico-term.
 *
 * pico-term is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * pico-term is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with pico-term.
 * If  not, see <https://www.gnu.org/licenses/>.
 */

#include <stdint.h>

/*
 * Initialisation Functions
 */


void tb_init(void* output_buffer, uint32_t output_length);

void tb_destroy();

/*
 * Functions for the writing of output data and reading of input data.
 */

uint32_t tb_write(void const* buffer, uint32_t buffsize);

void tb_flush();

/*
 * Internal functions for providing the internal ability to read the data in the output buffer
 * and to write the data to the input buffer.
 */

/*
 * Current size of data to write.
 */
uint32_t _tb_write_size();

uint32_t _tb_send(uint32_t (*write_cb)(void *buf, uint32_t bufsize), uint32_t size);

void _tb_flush(void (*flush_cb)());