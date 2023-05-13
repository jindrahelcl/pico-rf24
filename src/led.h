#ifndef __REGGNOX_LED_
#define __REGGNOX_LED_

typedef struct {
  uint gpio;
  bool state;

} led_t;

void led_init(led_t* led, uint gpio);
void led_destroy(led_t* led);

void led_toggle(led_t* led);
void led_on(led_t* led);
void led_off(led_t* led);

#endif
