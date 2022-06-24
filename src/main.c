#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "i2s_input.pio.h"
#include "button.h"
#include "storage.h"
#include "serial.h"

#define BUTTON_PIN 3
#define MIC_WS_PIN 5
#define MIC_CLK_PIN 6
#define MIC_SD_PIN 7

#define SERIAL_START_RECORD "start"
#define SERIAL_STOP_RECORD "stop"

#define DISC_FREQ (float )16000

#define SD_RETRY_TIMEOUT_MS 5 * 1000 // 5 sec

#define DMA_BUF_LEN 128

uint32_t buf[DMA_BUF_LEN];
int dma_chan;
bool isReady = false;
bool isRecording = false;

void dma_handler(){
	storage_setup_output_chunk(buf, DMA_BUF_LEN);
	dma_hw->ints0 = 1u << dma_chan;
	dma_channel_set_write_addr(dma_chan, buf, true);
	isReady = true;
}

void init_dma_pio_read(PIO pio, uint sm){
	dma_chan = dma_claim_unused_channel(true);

	dma_channel_config c = dma_channel_get_default_config(dma_chan);
	channel_config_set_transfer_data_size(&c, DMA_SIZE_32);
	channel_config_set_read_increment(&c, false);
	channel_config_set_write_increment(&c, true);
	channel_config_set_dreq(&c, pio_get_dreq(pio, sm, false));

	dma_channel_configure(
			dma_chan,
			&c,
			buf,
			&pio0_hw->rxf[sm],
			DMA_BUF_LEN,
			false
	);

	dma_channel_set_irq1_enabled(dma_chan, true);
	irq_set_exclusive_handler(DMA_IRQ_1, dma_handler);
	irq_set_enabled(DMA_IRQ_1, true);

}

void init_pio_i2s(){
	PIO pio = pio0;
	uint offset = pio_add_program(pio, &i2s_input_program);
	uint sm = pio_claim_unused_sm(pio, true);
	pio_i2s_input_init(pio, sm, offset, MIC_CLK_PIN, MIC_WS_PIN, MIC_SD_PIN, DISC_FREQ);
	init_dma_pio_read(pio, sm);
}

void start_record(){
	if(isRecording == true){
		return;
	}
	isRecording = true;
	if(storage_create_file() != 1){
		UART_LOG("Can't create file\n\r");
		return;
	}
	irq_set_enabled(DMA_IRQ_1, true);
	dma_channel_start(dma_chan);
}

void stop_record(){
	if(isRecording == false){
		return;
	}
	isRecording = false;
	irq_set_enabled(DMA_IRQ_1, false);
	dma_channel_abort(dma_chan);
	storage_write_data_chunk();
	isReady = false;
	UART_LOG("Record stopped\n\r");
}

void process_serial_message(UART_Message_t * msg){
	if(strncmp((char*) msg->msg, SERIAL_START_RECORD, sizeof(SERIAL_START_RECORD) - 1) == 0){
		start_record();
		return;
	}
	if(strncmp((char*) msg->msg, SERIAL_STOP_RECORD, sizeof(SERIAL_STOP_RECORD) - 1) == 0){
		stop_record();
		return;
	}
	UART_LOG("Type \"start\" to start record, \"stop\" to stop record\n\r");
}

int main(){
	UART_Message_t *msg;
	serial_init_uart();
	while (storage_sdcard_init() != 1){
		UART_LOG("No SD card.\n Insert valid SD card\n\r");
		sleep_ms(SD_RETRY_TIMEOUT_MS);
	}
	init_pio_i2s();
	button_init(BUTTON_PIN, start_record, stop_record);

	while(1){
		button_check_state();
		if(isReady == true){
			storage_write_data_chunk();
			isReady = false;
		}
		msg = serial_get_next_message();
		if(msg != NULL){
			process_serial_message(msg);
			destroy_message(msg);
		}
	}

}