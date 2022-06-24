#ifndef MY_BUTTON_H
#define MY_BUTTON_H

typedef void (*ButtonCB_t)();

void button_check_state();
void button_init(int pin, ButtonCB_t shortPress, ButtonCB_t longPress);


#endif // MY_BUTTON_H