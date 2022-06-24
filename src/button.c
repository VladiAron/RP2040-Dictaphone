#include <stdio.h>
#include <stdint.h>

#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "button.h"

#define DEBOUNCE_TIMEOUT 50000 // 50 ms
#define LONG_PRESS_PERIOD 1000000 // 1 sec

int gpio;
uint32_t lastTriggerTime;
uint32_t lastImpulseTime;
ButtonCB_t on_short_press;
ButtonCB_t on_long_press;

void stub_cb(){
	return;
}

static void button_callback(uint pin, uint32_t event){
	if(time_us_32() - lastTriggerTime < DEBOUNCE_TIMEOUT){
		return;
	}
	if(pin == gpio){
		if(event & GPIO_IRQ_EDGE_FALL){
			lastTriggerTime = time_us_32();
		}
		if(event & GPIO_IRQ_EDGE_RISE){
			lastImpulseTime = time_us_32() - lastTriggerTime;
		}
	}
}

void button_check_state(){
	if(lastImpulseTime > LONG_PRESS_PERIOD){
		on_long_press();
	}else if(lastImpulseTime > DEBOUNCE_TIMEOUT){
		on_short_press();
	}
	lastImpulseTime = 0;
}

void button_init(int pin, ButtonCB_t shortPress, ButtonCB_t longPress){
	gpio = pin;
	gpio_init(pin);
	gpio_set_dir(pin, GPIO_IN);
	gpio_pull_up(pin);
	gpio_set_irq_enabled_with_callback(pin, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL , true, &button_callback);
	on_short_press = stub_cb;
	on_long_press = stub_cb;

	if(shortPress != NULL) {
		on_short_press = shortPress;
	}
	if(longPress != NULL){
		on_long_press = longPress;
	}

}