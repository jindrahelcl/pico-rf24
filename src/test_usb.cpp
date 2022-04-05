#include <stdio.h>
#include "pico/stdlib.h"

int main() {
  stdio_init_all();

  while (!stdio_usb_connected()) {
    sleep_ms(100);
  }

  printf("no konečně vole");

  return 0;
}