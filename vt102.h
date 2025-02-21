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

/**
 * VT102 Output Utility Functions
 */

#ifndef VT102_H
#define VT102_H

#include <stdint.h>

// Types

enum vt102_event_type
{
    connect, disconnect, none, character, control, alt, special
};

static inline const char* vt102_event_type_to_string(enum vt102_event_type type)
{
    switch (type)
    {
        case connect: return "connect";
        case disconnect: return "disconnect";
        case none: return "none";
        case character: return "character";
        case control: return "control";
        case alt: return "alt";
        case special: return "special";
        default: return "unknown";
    }
}

enum special_key
{
    home, insert, delete, end, page_up, page_down, up, down, right, left,
    f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12
};

static inline const char* special_key_to_string(enum special_key key)
{
    switch (key)
    {
        case home: return "home";
        case insert: return "insert";
        case delete: return "delete";
        case end: return "end";
        case page_up: return "page_up";
        case page_down: return "page_down";
        case up: return "up";
        case down: return "down";
        case right: return "right";
        case left: return "left";
        case f1: return "f1";
        case f2: return "f2";
        case f3: return "f3";
        case f4: return "f4";
        case f5: return "f5";
        case f6: return "f6";
        case f7: return "f7";
        case f8: return "f8";
        case f9: return "f9";
        case f10: return "f10";
        case f11: return "f11";
        case f12: return "f12";
        default: return "unknown";
    }
}

struct vt102_event
{
    enum vt102_event_type event_type;
    char character;
};

typedef struct vt102_event vt102_event;

// External Functions - All start vt102

/*
 * Reset to Initial State
 */
void vt102_ris();

/*
 * Erase Display
 */
void vt102_erase_display();

/*
 * Cursor Position
 */
void vt102_cup(char* line, char* column);

// Internal Functions - All start _vt102

/*
 * Write Buffer to Terminal
 */
uint32_t _vt102_write (void const* buffer, uint32_t bufsize);

/*
 * Write a single character to the terminal
 */
uint32_t _vt102_write_char (char ch);

/*
 * Write zero terminated String to terminal
 */
uint32_t _vt102_write_str (char const* str);

/*
 * Flush output to terminal
 */
void _vt102_write_flush (void);

#endif // VT102_H
