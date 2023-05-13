#include <stdbool.h>
#include <RF24.h>

// predefine addresses for two nodes
uint8_t pipes[][6] = {"1Node", "2Node"};

void radio_init(radio_t* r, uint gpio_ce, uint gpio_csn, uint8_t* readAddr, uint8_t* writeAddr) {
  r->gpio_ce = gpio_ce;
  r->gpio_csn = gpio_csn;
  r->backend = new RF24(gpio_ce, gpio_csn);

  r->readAddr = readAddr;
  r->writeAddr = writeAddr;
}


bool radio_open(radio_t* r) {
  if(!r->backend->begin()) {
    return false;
  }

  radio->setPALevel(RF24_PA_MAX);
  radio->setPayloadSize(sizeof(float));
  radio->setAutoAck(false);

  radio->openReadingPipe(1, r->readAddr);
  radio->openWritingPipe(r->writeAddr);

  return true;
}


void radio_destroy(radio_t* r) {
  delete r->backend;
}
