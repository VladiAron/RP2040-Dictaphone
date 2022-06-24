//
// Created by aron on 24.06.22.
//

#ifndef RECORDER_SERIAL_H
#define RECORDER_SERIAL_H

#endif //RECORDER_SERIAL_H

#include "hardware/uart.h"

#define UART_ID uart0
#define BAUD_RATE 115200
#define DATA_BITS 8
#define STOP_BITS 1
#define PARITY    UART_PARITY_NONE

#define SERIAL_BUF_LEN 1024

#define UART_TX_PIN 0
#define UART_RX_PIN 1

#define UART_LOG(x) uart_puts(UART_ID, x)

typedef struct UART_Message_s {
	struct UART_Message_s * next;
	uint len;
	uint8_t * msg;
}UART_Message_t;

void destroy_message(UART_Message_t * ptr);
UART_Message_t * serial_get_next_message();
void serial_init_uart();

