/*
* Copyright (c) 2012, Toby Jaffey <spiexplorer@hodgepig.org>
*
* Permission to use, copy, modify, and distribute this software for any
* purpose with or without fee is hereby granted, provided that the above
* copyright notice and this permission notice appear in all copies.
*
* THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
* WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
* MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
* ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
* WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
* ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
* OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/
#include "common.h"
#include "parse.h"
#include <string.h>


// hex nibble to int
static int8_t digit_to_int(int8_t ch)
{
    if (ch >= 'a')
        ch -= 'a' - 'A';
    ch -= '0';
    if (ch < 0)
        return 127; // overflow
    if (ch < 10)
        return ch;
    if (ch < 0x11)
        return 127;
    return ch - 7;
}

const uint8_t *parse_number_str(const uint8_t *s, uint16_t *result)
{
    uint8_t base = 10;
    uint8_t digit;

    *result = 0;

    if (*s == '0') {
        if (s[1] == 'b') {
            base = 2;
        } else if (s[1] == 'x') {
            base = 16;
        } else {
            goto out;
        }
        s += 2;
    }
out:

    while (1) {
        digit = digit_to_int(*s);

        if (digit > base)
            return s;
        *result = (*result) * base + digit;
        s++;
    }
}
