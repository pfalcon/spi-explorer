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
#include "hiz.h"
#include "spi.h"
#include <ctype.h>

// Use duplex mode for bus transfers
static char duplex;
static struct Bus *buses[] = {&hiz_bus, &spi_bus};
static struct Bus *current_bus;

static void set_bus(int bus)
{
    if (current_bus && current_bus->exit)
        current_bus->exit();
    current_bus = buses[bus];
    console_prompt = current_bus->prompt;
    current_bus->init();
}

void shell_init(void)
{
    set_bus(BUS_HIZ);
}

static void bus_spi_start(void)
{
    current_bus->start();

    console_puts("CS ENABLED");
    console_newline();
}

static void bus_spi_stop(void)
{
    current_bus->stop();

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
    uint8_t r = current_bus->xact(c);

    console_puts("WRITE: 0x");
    console_puthex8(c);
    console_newline();
    if (duplex)
        bus_dump_read(r);
}

static void bus_spi_read(void)
{
    uint8_t c;
    c = current_bus->xact(0xFF);
    bus_dump_read(c);
}

BOOL match(const uint8_t *s, char *pat)
{
    while (*pat) {
        if (*s++ != *pat++)
            return FALSE;
    }
    return TRUE;
}

const uint8_t *eval_pin_command(const uint8_t *s)
{
    // Set port pin command
    uint8_t mask;
    volatile uint8_t *port;

    if (s[2] != '.')
        goto syntax_error;

    mask = 1 << (s[3] - '0');
    switch (s[1]) {
    case '1':
        port = &P1IN;
        break;
    case '2':
        port = &P2IN;
        break;
    default:
        goto syntax_error;
    }

    if (s[4] == '=') {
        // PxDIR
        port[2] |= mask;
        // PxOUT
        if (s[5] == '0')
            port[1] &= ~mask;
        else
            port[1] |= mask;
        s += 6;
    } else if (s[4] == '?') {
        // PxDIR
        port[2] &= ~mask;
        console_puts("READ: ");
        // PxIN
        if (port[0] & mask)
            console_putc('1');
        else
            console_putc('0');
        console_newline();
        s += 5;
    }
    return s;

syntax_error:
    console_puts("BadCmd");
    console_newline();
    return NULL;
}

const uint8_t *eval_single_bus_command(const uint8_t *s);
void eval_bus_commands(const uint8_t *s)
{
    while (*s) {
        if (*s == ' ' || *s == '\t' || *s == ',') {
            s++;
            continue;
        }
        if (!(s = eval_single_bus_command(s))) {
            break;
        }
    }
}


const uint8_t *eval_single_bus_command(const uint8_t *s)
{
    uint16_t num;
    uint16_t repeat = 1;
    uint8_t cmd;

    // Process non-repeatable commands
    switch (*s) {
    case '{':
        duplex = 1;
    case '[':
        bus_spi_start();
        return s + 1;
    case '}':
    case ']':
        bus_spi_stop();
        duplex = 0;
        return s + 1;
    case 'p':
        return eval_pin_command(s);
    }

    // Process repeatable commands
    if (isdigit(*s)) {
        s = parse_number_str(s, &num);
        cmd = '0';
    } else if (*s == 'r') {
        cmd = *s++;
    } else if (*s == '&') {
        cmd = *s++;
    } else {
        goto syntax_error;
    }

    if (*s == ':') {
        s = parse_number_str(s + 1, &repeat);
    }

    while(repeat--) {
        switch (cmd) {
        case '0':
            bus_spi_write((uint8_t)num);
            break;
        case 'r':
            bus_spi_read();
            break;
        case '&':
            //delay_1us();
            break;
        }
    }
    return s;

syntax_error:
    console_puts("BadCmd");
    console_newline();
    return NULL;
}

void shell_eval(const uint8_t *s, uint16_t len)
{
    // Process directives (start at the beginning of line, take whole line)
    if (match(s, "echo o")) {
        console_echo = TRUE;
        if (s[6] == 'f')
            console_echo = FALSE;
        return;
    } else if (match(s, "spi")) {
        set_bus(BUS_SPI);
        return;
    } else if (match(s, "hiz")) {
        set_bus(BUS_HIZ);
        return;
    } else if (match(s, "peek")) {
        uint16_t addr;
        parse_number_str(s + 5, &addr);
        bus_dump_read(*(uint8_t*)addr);
        return;
    } else {
        // No directive - process bus commands
        eval_bus_commands(s);
    }
}
