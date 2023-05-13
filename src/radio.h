#ifndef __REGGNOX_RADIO_
#define __REGGNOX_RADIO_

#include <RF24.h>

typedef enum {
  FIRST,
  SECOND
} node_t;

typedef struct {

  bool transmitting;
  RF24* backend;

  uint gpio_ce;
  uint gpio_csn;

  uint8_t* readAddr;
  uint8_t* writeAddr;

} radio_t;


void radio_init(radio_t* r, uint gpio_ce, uint gpio_csn);
void radio_destroy(radio_t* r);




#endif
