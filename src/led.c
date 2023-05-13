#include "pico/stdlib.h"
#include "led.h"

void led_init(led_t* led, uint gpio) {
  led->gpio = gpio;
  led->state = 0;
  gpio_init(gpio);
  gpio_set_dir(gpio, GPIO_OUT);
}

void led_destroy(led_t* led) { };

void led_on(led_t* led) {
  led->state = 1;
  gpio_put(led->gpio, led->state);
}

void led_off(led_t* led) {
  led->state = 0;
  gpio_put(led->gpio, led->state);
}

void led_toggle(led_t* led) {
  led->state = !led->state;
  gpio_put(led->gpio, led->state);
}
