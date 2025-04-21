/*
 * terminal_handler.c - Terminal Handler
 *
 * This file is part of the Pico Playground project.
 *
 * Copyright 2024, Darran A Lofthouse
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>

#include "bsp/board.h"
#include "pico/stdlib.h"
#include "tusb.h"

#include "terminal_buffer.h"
#include "terminal_handler.h"
#include "vt102.h"

/*
 * Called after each loop once all data has been sent back to the client.
 */
void handle(struct vt102_event *event);

static uint32_t write_cb(void* buf, uint32_t bufsize)
{
    return tud_cdc_n_write(CDC_INTF, buf, bufsize);
}

static void flush_cb()
{
    tud_cdc_n_write_flush(CDC_INTF);
}

#define READ_SIZE 5

struct read_status
{
    uint8_t current_read_pos;
    char *current_read;
};

static uint32_t decode_event(struct read_status *read_status, struct vt102_event *vt102_event);

#define TERMINAL_CONTEXT_ID 0xAA
struct terminal_context
{
    char id;
    bool connected;
    unsigned char write_buffer[WRITE_BUFFER_LENGTH];
    uint32_t largest_send;
    uint32_t largest_available;
    char read_buffer[READ_SIZE];
    struct read_status read_status;
    vt102_event_handler event_handler;
    void *hand_back;
};

void *terminal_handler_init()
{
    struct terminal_context *context = malloc(sizeof(struct terminal_context));
    context->id = TERMINAL_CONTEXT_ID;

    context->connected = false;
    context->largest_send = 0;
    context->largest_available = 0;
    context->read_status.current_read_pos = 0;
    context->read_status.current_read = context->read_buffer;

    return context;
}

bool terminal_handler_begin(void *context, vt102_event_handler event_handler, void *hand_back)
{
    struct terminal_context *term_context = (struct terminal_context *)context;
    if (term_context->id != TERMINAL_CONTEXT_ID)
    {
        printf("Invalid context passed to terminal_handler_begin 0x%02x\n", term_context->id);
        return false;
    }

    term_context->event_handler = event_handler;
    term_context->hand_back = hand_back;
}

void terminal_handler_run(void *context)
{
    struct terminal_context *term_context = (struct terminal_context *)context;
    if (term_context->id != TERMINAL_CONTEXT_ID)
    {
        printf("Invalid context passed to terminal_handler_run 0x%02x\n", term_context->id);
        return;
    }

    tud_task();

    bool connected = tud_cdc_n_connected(CDC_INTF);
    if (connected)
    {
        if (!term_context->connected)
        {
            // We have connected.
            term_context->connected = true;
            struct vt102_event event = {connect, 0x00};
            tb_init(term_context->write_buffer, WRITE_BUFFER_LENGTH);
            term_context->event_handler(&event, term_context->hand_back);
        }

        if (_tb_write_size() > term_context->largest_send)
        {
            term_context->largest_send = _tb_write_size();
            printf("Largest send so far %d\n", term_context->largest_send);
        }

        // If we have data to write we will write is all before handle() is called.
        if (_tb_write_size() && tud_cdc_n_write_available(CDC_INTF))
        {
            // We have data to send AND there is room on the buffer.
            _tb_send(write_cb, tud_cdc_n_write_available(CDC_INTF));

            if (!_tb_write_size())
            {
                // Add data written, check if it needs a flush.
                _tb_flush(flush_cb);
            }
        }

        struct vt102_event event = {none, 0x00};
        if (!_tb_write_size())
        {
            // Stage 1 - Reading data into current_read
            if (term_context->read_status.current_read_pos < READ_SIZE)
            {
                uint32_t bytes_available = tud_cdc_n_available(CDC_INTF);

                uint32_t bytes_read = tud_cdc_n_read(CDC_INTF,
                                                     term_context->read_status.current_read +
                                                         term_context->read_status.current_read_pos,
                                                     READ_SIZE - term_context->read_status.current_read_pos);
                term_context->read_status.current_read_pos += bytes_read;
            }

            if (decode_event(&term_context->read_status, &event))
            {
                // We have a complete event to handle.
                term_context->event_handler(&event, context);
            }
        }
    }
    else
    {
        if (term_context->connected)
        {
            // We have disconnected.
            term_context->connected = false;
            struct vt102_event event = {disconnect, 0x00};
            term_context->event_handler(&event, term_context->hand_back);
            tb_destroy(); // handle_disconnected may have wanted to drain the remaining input data.
        }
    }
}

static uint32_t decode_event(struct read_status *read_status, struct vt102_event *event)
{
    // Stage 2 - Use some, all, or none of the data in current_read
    // to process an event.
    uint8_t current_read_pos = read_status->current_read_pos;
    char *current_read = read_status->current_read;
    if (current_read_pos > 0)
    {
        uint8_t chars_used = 0;
        if (current_read[0] == 0x1B)
        {
            // We have an escape sequence.
            if (current_read_pos == 1)
            {
                // We need to read more data to determine the escape sequence.
                return -1;
            }
            else
            {
                // We have enough data to determine the escape sequence.
                if (current_read[1] == 0x1B)
                {
                    chars_used = 1; // We will assume the first ESC is a redundant control character.
                }
                else if (current_read[1] == 0x5B)
                {
                    // We have a function key sequence.
                    if (current_read_pos < 3)
                    {
                        // We need to read more data to determine the function key.
                        return -1;
                    }
                    else if (current_read[2] >= 0x41 && current_read[2] <= 0x44)
                    {
                        // Arrow Key
                        event->event_type = special;
                        event->character = current_read[2] - (0x41 - up);
                        chars_used = 3;
                    }
                    else if (current_read_pos < 4)
                    {
                        // We need to read more data to determine the function key.
                        return -1;
                    }
                    else if (current_read[2] >= 0x31 && current_read[2] <= 0x36 && current_read[3] == 0x7E)
                    {
                        event->event_type = special;
                        event->character = current_read[2] - (0x31 - home);
                        chars_used = 4;
                    }
                    else if (current_read_pos < 5)
                    {
                        // We need to read more data to determine the function key.
                        return -1;
                    }
                    else
                    {
                        // We have enough data to determine the function key.
                        if (current_read[1] == 0x5B && current_read[2] == 0x31 && current_read[4] == 0x7E && current_read[3] >= 0x36 && current_read[3] <= 0x3D)
                        {
                            // We have a function key - F5 and up.
                            event->event_type = special;
                            event->character = current_read[3] - (0x36 - f5);
                            chars_used = 5;
                        }
                        else
                        {
                            // We have an unknown function key.
                            chars_used = 5;
                        }
                    }
                }
                else if (current_read[1] == 0x4f)
                {
                    if (current_read_pos == 2)
                    {
                        // We need to read more data to determine the escape sequence.
                        return -1;
                    }
                    else
                    {
                        // We have enough data to determine the escape sequence.
                        if (current_read[2] >= 0x50 && current_read[2] <= 0x53)
                        {
                            // We have a function key - F1 to F4.
                            event->event_type = special;
                            event->character = current_read[2] - (0x50 - f1); // Convert to ASCII.
                            chars_used = 3;
                        }
                        else if (current_read[2] >= 0x61 && current_read[2] <= 0x7A)
                        {
                            // We have an alt key.
                            event->event_type = alt;
                            event->character = current_read[2] - 0x1F; // Convert to ASCII.
                            chars_used = 3;
                        }
                        else
                        {
                            // We have an unknown function key.
                            chars_used = 3;
                        }
                    }
                }
                else if (current_read[1] >= 0x61 && current_read[1] <= 0x7A)
                {
                    // We have a control sequence.
                    event->event_type = alt;
                    event->character = current_read[1] - 0x20; // Convert to ASCII.
                    chars_used = 2;
                }
                else
                {
                    // We have an unknown escape sequence.
                    chars_used = 2;
                }
            }
        }
        else if (current_read[0] >= 0x20 && current_read[0] <= 0x7E)
        {
            // We have a printable character.
            event->event_type = character;
            event->character = current_read[0];
            chars_used = 1;
        }
        else if (current_read[0] <= 0x1A)
        {
            // We have a control character.
            event->event_type = control;
            event->character = current_read[0] + 0x40; // Convert to ASCII.
            chars_used = 1;
        }
        else
        {
            // We have an unknown character.
            chars_used = 1;
        }

        // Stage 3 - Move the data in current_read to the start of the buffer.
        if (chars_used > 0)
        {
            // Move the remaining data in current_read to the start of the buffer.
            for (uint8_t i = chars_used; i < read_status->current_read_pos; i++)
            {
                read_status->current_read[i - chars_used] = read_status->current_read[i];
            }
            read_status->current_read_pos -= chars_used;
        }
    }

    return 1;
}

