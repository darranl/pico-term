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
#include <stdbool.h>
#include <string.h>

static struct terminal_buffer
{
    void* output_buffer;    // The buffer to hold data to send to the client.
    uint32_t output_start;  // The index the data begins at.
    uint32_t output_end;    // The index of the position to add data.
    uint32_t output_length; // Total size of the output buffer.

    bool flush;
} _terminal_buffer;

void tb_init(void* write_buffer, uint32_t write_length)
{
    _terminal_buffer.output_buffer = write_buffer;
    _terminal_buffer.output_start = 0;
    _terminal_buffer.output_end = 0;
    _terminal_buffer.output_length = write_length;
    _terminal_buffer.flush = false;
}

void tb_destroy()
{
    _terminal_buffer.output_buffer = 0;
    _terminal_buffer.output_start = 0;
    _terminal_buffer.output_end = 0;
    _terminal_buffer.output_length = 0;
    _terminal_buffer.flush = false;
}

uint32_t tb_write(void const* buffer, uint32_t buffsize)
{
    uint32_t available = _terminal_buffer.output_length - _terminal_buffer.output_end;
    uint32_t towrite = buffsize <= available ? buffsize : available;
    memcpy(_terminal_buffer.output_buffer + _terminal_buffer.output_end, buffer, towrite);
    _terminal_buffer.output_end += towrite;
    return towrite;
}

void tb_flush()
{
    _terminal_buffer.flush = true;
}

/*
 * Internal Functions
 */

uint32_t _tb_write_size()
{
    return _terminal_buffer.output_end - _terminal_buffer.output_start;
}

uint32_t _tb_send(uint32_t (*write_cb)(void *buf, uint32_t bufsize), uint32_t size)
{
    uint32_t available = _tb_write_size();
    uint32_t tosend = available >= size ? size : available;

    uint32_t sent = write_cb(_terminal_buffer.output_buffer + _terminal_buffer.output_start,
                             tosend);

    if (sent < available)
    {
        // Just move the start offset along for now.
        _terminal_buffer.output_start += sent;
    }
    else
    {
        // We sent it all.
        _terminal_buffer.output_start = 0;
        _terminal_buffer.output_end = 0;
    }

    return sent;
}

void _tb_flush(void (*flush_cb)())
{
    if (_terminal_buffer.flush)
    {
        _terminal_buffer.flush = false;
        flush_cb();
    }
}
