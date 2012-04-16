#ifndef UART_H
#define UART_H 1

#include "common.h"

#define TXD BIT1 // TXD on P1.1
#define RXD BIT2 // RXD on P1.2

#define FCPU 1000000
#define BAUDRATE 9600

#define BIT_TIME        (FCPU / BAUDRATE)
#define HALF_BIT_TIME   (BIT_TIME / 2)

void uart_init(void);
BOOL uart_getc(uint8_t *c);
void uart_putc(uint8_t c);

#endif

