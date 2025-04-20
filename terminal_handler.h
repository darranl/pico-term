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

#ifndef TERMINAL_HANDLER_H
#define TERMINAL_HANDLER_H

#include "term/vt102.h"

#define CDC_INTF 0
#define WRITE_BUFFER_LENGTH 2048

typedef void (*vt102_event_handler)(vt102_event *event, void *context);

void *terminal_handler_init();
bool terminal_handler_begin(void *context, vt102_event_handler event_handler, void *hand_back);
void terminal_handler_run(void *context);

#endif // TERMINAL_HANDLER_H
