/*
* uart.h
*
* Created: 4/2/2019 1:34:20 PM
*  Author: mw
*/


#ifndef UART_H_
#define UART_H_

//sending char to UART stdio version
void uart_putchar(char c, FILE *stream);

//sending char to UART
void uart_write(char c);

//reading from UART stdio version
char uart_getchar(FILE *stream);

//initializing UART baud rate have to be set in uart.c
void uart_init(void);

// magic to route UART to stdio
FILE uart_output = FDEV_SETUP_STREAM((void*)uart_putchar, NULL, _FDEV_SETUP_WRITE);
FILE uart_input = FDEV_SETUP_STREAM(NULL, (void*)uart_getchar, _FDEV_SETUP_READ);

#endif /* UART_H_ */