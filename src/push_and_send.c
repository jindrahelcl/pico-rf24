#include <stdio.h>

#include "pico/stdlib.h"
#include "hardware/gpio.h"

#include "microphone.h"
#include "button.h"

const int BUTTON_PIN = 15;


void button_callback(uint gpio, uint32_t event_mask) {

  if(gpio == BUTTON_PIN) {

  }
}


int main(int argc, char* argv[]) {
  stdio_init_all();

  button_t button;
  button_init(&button, BUTTON_PIN);

  irq_handler irq;
  irq_handler_init(&irq, GPIO_IRQ_EDGE_FALL);

  button_set_irq_handler(&button, &irq);
  button_set_callback(&button, button_callback);

  while (!stdio_usb_connected()) {
    tight_loop_contents();
  }

  while(true) {


  }



}
