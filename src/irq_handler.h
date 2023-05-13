#ifndef __REGGNOX_IRQ_HANDLER_H_
#define __REGGNOX_IRQ_HANDLER_H_

#include "hardware/gpio.h"

typedef struct {
  // this struct provides an iterface between the shared GPIO IRQ handler and
  // the individual GPIO IRQ clients.

  uint32_t event_mask;


} irq_handler;

void irq_handler_init(irq_handler* handler, uint32_t event_mask );
void irq_handler_add_callback(irq_handler* handler, uint gpio, gpio_irq_callback callback);
void irq_handler_remove_callback(irq_handler* handler, uint gpio);

#endif
