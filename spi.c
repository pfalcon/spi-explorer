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
#include "spi.h"


void spi_init(void)
{
    P1DIR |= SCLK | SDO | CS;
    P1DIR &= ~SDI;

    // enable SDI, SDO, SCLK, master mode, MSB, output enabled, hold in reset
    USICTL0 = USIPE7 | USIPE6 | USIPE5 | USIMST | USIOE | USISWRST;

    // SMCLK / 128 = ~7.8KHz
    USICKCTL = USISSEL_2 | USIDIV_7;

    // clock phase
    USICTL1 |= USICKPH;

    // release from reset
    USICTL0 &= ~USISWRST;
}

void spi_exit(void)
{
    USICTL0 = USISWRST;
    P1DIR &= ~(SCLK | SDO | CS | SDI);
}

void spi_cs_assert(void)
{
    // assert CS
    P1OUT &= ~CS;
}

void spi_cs_deassert(void)
{
    // deassert CS
    P1OUT |= CS;
}

uint8_t spi_write8(uint8_t c)
{
    USISRL = c;
    // clear interrupt flag
    USICTL1 &= ~USIIFG;
    // set number of bits to send, begins tx
    USICNT = 8;

    // wait for tx
    while(!(USICTL1 & USIIFG));

    c = USISRL;

    return c;
}

struct Bus spi_bus = {
    .prompt = "SPI",
    .init = spi_init,
    .exit = spi_exit,
    .start = spi_cs_assert,
    .stop = spi_cs_deassert,
    .xact = spi_write8,
};
