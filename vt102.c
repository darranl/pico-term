
/**
 * Implementation of VT102 Utility Functions
 *
 * This file defines the constants as octal values
 * as these are used in the VT102 documentation.
 *
 * i.e. Start with 0
 */

#include "vt102.h"
#include "tusb.h"
#include "terminal_buffer.h"

const char ERASE_DISPLAY[] = { 033, 0133, 060, 0112};
const char RIS[] = { 033, 0143 };

// External Functions

void vt102_ris()
{
    _vt102_write(RIS, 2);
}

void vt102_erase_display()
{
    _vt102_write(ERASE_DISPLAY, 4);
}

void vt102_cup(char* line, char* column)
{
    _vt102_write_char(033);
    _vt102_write_char(0133);
    _vt102_write_str(line);
    _vt102_write_char(073);
    _vt102_write_str(column);
    _vt102_write_char(0110);
    // Don't flush, something likely to be written immediately after.
}

// Internal Functions
// TODO An API will be added to these can output to different destinations:
//    - STDOUT
//    - USB / CDC
//    - UART
//    - Others?

uint32_t _vt102_write (void const* buffer, uint32_t bufsize)
{
    return tb_write(buffer, bufsize);
}

uint32_t _vt102_write_char (char ch)
{
    return _vt102_write(&ch, 1);
}

uint32_t _vt102_write_str (char const* str)
{
    uint32_t length = strlen(str);
    return _vt102_write(str, length);
}

void _vt102_write_flush (void)
{
    tb_flush();
}
