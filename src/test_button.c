#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "button.h"

const uint LED_PIN = 25;
const uint BUTTON_PIN = 15;

void on_change(button_t* button) {
  printf("Button on pin %d changed its state to %d\n", button->pin, button->state);

  if(button->state) {
    printf("pressed\n");
    gpio_put(LED_PIN, 1);
  }
  else {
    printf("released\n");
    gpio_put(LED_PIN, 0);
  }
}

int main(void) {
  stdio_init_all();

  printf("hello button!\n");

  gpio_init(LED_PIN);
  gpio_set_dir(LED_PIN, GPIO_OUT);

  create_button(BUTTON_PIN, on_change);
  while (true) {}
}
