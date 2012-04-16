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
#include "console.h"
#include "parse.h"
#include "shell.h"
#include "spi.h"

enum
{
    STATE_SEARCHING,
    STATE_IN_TOKEN,
};

// Use duplex mode for bus transfers
static char duplex = 1; // Need to have this initilized, because if this goes to BSS, something trashes it

void shell_init(void)
{
}

static void bus_spi_start(void)
{
    spi_cs_assert();

    console_puts("CS ENABLED");
    console_newline();
}

static void bus_spi_stop(void)
{
    spi_cs_deassert();

    console_puts("CS DISABLED");
    console_newline();
}

static void bus_dump_read(uint8_t c)
{
    console_puts("READ: 0x");
    console_puthex8(c);
    console_newline();
}

static void bus_spi_write(uint8_t c)
{
    uint8_t r = spi_write8(c);
    console_puts("WRITE: 0x");
    console_puthex8(c);
    console_newline();
    if (duplex)
        bus_dump_read(r);
}

static void bus_spi_read(void)
{
    uint8_t c;
    c = spi_write8(0xFF);
    bus_dump_read(c);
}


static BOOL handle_command(const uint8_t *start, const uint8_t *end, BOOL firstToken, const uint8_t *line_end)
{
    uint16_t num;
    const uint8_t *ptr = start;
    uint16_t repeat = 1;

    // handle repeat commands, A:5
    while(ptr != end)
    {
        if (*ptr == ':' && ptr-start > 0)   // found at least one character followed by ':'
        {
            if (parse_number(ptr+1, end - (ptr+1), &repeat))
            {
                end = ptr;    // discard the ":X" part
                break;
            }
            else
                repeat = 0; // parse_number is destructive
        }
        ptr++;
    }
    
    // handle write command (a number)
    if (parse_number(start, end-start, &num))
    {
        while(repeat--)
            bus_spi_write((uint8_t)num);
        return FALSE;
    }
    else
    {
        if (end-start == 1 && (start[0] == 'r' || start[0] == 'R'))
        {   // handle read command
            while(repeat--)
                bus_spi_read();
            return FALSE;
        }
        else if (start[0] == 's' && start[1] == 'p' && start[2] == 'i')
        {
            spi_init();
            return TRUE;
        }
        else if (start[0] == 'p')
        {
            // Set port pin command
            uint8_t mask;
            if (start[2] != '.')
                goto syntax_error;
            mask = 1 << (start[3] - '0');
            if (start[4] == '=') {
                if (start[1] == '1') {
                    P1DIR |= mask;
                    if (start[5] == '0')
                        P1OUT &= ~mask;
                    else
                        P1OUT |= mask;
                }
                else if (start[1] == '2') {
                    P2DIR |= mask;
                    if (start[5] == '0')
                        P2OUT &= ~mask;
                    else
                        P2OUT |= mask;
                }
            }
            else if (start[4] == '?') {
                if (start[1] == '1') {
                    P1DIR &= ~mask;
                    console_puts("READ: ");
                    if (P1IN & mask)
                        console_putc('1');
                    else
                        console_putc('0');
                    console_newline();
                }
                else if (start[1] == '2') {
                    P2DIR &= ~mask;
                    console_puts("READ: ");
                    if (P2IN & mask)
                        console_putc('1');
                    else
                        console_putc('0');
                    console_newline();
                }
            }
            else
                goto syntax_error;
            return FALSE;
        }
        else
        {   // handle other commands
            // ignore
        }
    }

syntax_error:
    console_puts("BadCmd");
    console_newline();

    return FALSE;
}


void shell_eval(const uint8_t *str, uint16_t len)
{
    uint8_t state = STATE_SEARCHING;
    uint8_t c;
    const uint8_t *end = str+len;
    const uint8_t *token_start = str;
    BOOL firstToken = TRUE;

    while(str != end)
    {
        c = *str;

        switch(state)
        {
            case STATE_SEARCHING:
                switch(c)
                {
                    case '#':   // comment
                        return; // ignore the rest of the line

                    case ' ':   // whitespace
                    case '\t':
                    case ',':
                        break;

                    case '{':
                        duplex = 1;
                    case '[':
                        firstToken = FALSE;
                        bus_spi_start();
                        break;

                    case ']':
                    case '}':
                        firstToken = FALSE;
                        bus_spi_stop();
                        duplex = 0;
                        break;

                    default:    // start of token
                        state = STATE_IN_TOKEN;
                        token_start = str;
                        break;
                }
                str++;  // next character
                break;  // end of case STATE_SEARCHING

            case STATE_IN_TOKEN:
                switch(c)
                {
                    case '{':
                        duplex = 1;
                    case ' ':   // end of token
                    case '\t':
                    case ',':
                    case '[':
                    case ']':
                    case '}':
                    case '#':
                        if (handle_command(token_start, str, firstToken, end))
                            return;
                        firstToken = FALSE;
                        state = STATE_SEARCHING;
                        break;

                    default:
                        // keep scanning token
                        str++;
                        break;
                }
        }
    }

    if (STATE_IN_TOKEN == state)
        handle_command(token_start, end, firstToken, end);
}



