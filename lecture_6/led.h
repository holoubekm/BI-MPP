#ifndef __LED_H
#define __LED_H

#define LED_R 1
#define LED_G 2
#define LED_B 3

void led_init();
void led_set(int led, int v);
void led_on(int led);
void led_off(int led);
void led_all_off();


#endif

