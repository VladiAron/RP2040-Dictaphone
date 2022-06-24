//
// Created by aron on 24.06.22.
//
#include <string.h>
#include <stdlib.h>
#include "serial.h"
#include "hardware/irq.h"
#include "hardware/gpio.h"

UART_Message_t * head;

uint8_t inputBuf[SERIAL_BUF_LEN];
int chars_rxed = 0;

UART_Message_t * create_new_message(uint8_t * buf, int len){
	UART_Message_t * new = malloc(sizeof(UART_Message_t));
	new->msg = malloc(sizeof(uint8_t) * len);
	new->len = len;
	new->next = NULL;
	memcpy(new->msg, buf, sizeof(uint8_t) * len);
	return new;
}

void destroy_message(UART_Message_t * ptr){
	free(ptr->msg);
	free(ptr);
}

void add_to_end(UART_Message_t * new){
	if(head == NULL){
		head = new;
		return;
	}
	UART_Message_t * ptr = head;

	while(ptr->next != NULL){
		ptr = ptr->next;
	}
	ptr->next = new;
}

void on_uart_rx() {
	while (uart_is_readable(UART_ID)) {
		uint8_t ch = uart_getc(UART_ID);
		if (uart_is_writable(UART_ID)) {
			if(ch == '\r'){
				uart_putc(UART_ID, '\n');
			}
			uart_putc(UART_ID, ch);
		}
		inputBuf[chars_rxed] = ch;
		chars_rxed++;

		if(ch == '\r' || chars_rxed == SERIAL_BUF_LEN - 1){
			inputBuf[chars_rxed + 1] = 0;
			add_to_end(create_new_message(inputBuf, chars_rxed));
			chars_rxed = 0;
			memset(inputBuf, 0, SERIAL_BUF_LEN);
		}
	}
}

UART_Message_t * serial_get_next_message(){
	if(head == NULL){
		return NULL;
	}
	UART_Message_t * ret = head;
	head = head->next;
	return ret;
}

void serial_init_uart(){
	uart_init(UART_ID, BAUD_RATE);

	gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
	gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
	uart_set_hw_flow(UART_ID, false, false);
	uart_set_fifo_enabled(UART_ID, false);

	irq_set_exclusive_handler(UART0_IRQ, on_uart_rx);
	irq_set_enabled(UART0_IRQ, true);

	uart_set_irq_enables(UART_ID, true, false);
}