#include <stdbool.h>
#include "irq_handler.h"

void irq_handler_init(irq_handler* handler, uint32_t event_mask) {
  handler->event_mask = event_mask;
}

void irq_handler_add_callback(irq_handler* handler, uint gpio, gpio_irq_callback callback) {
  gpio_set_irq_enabled_with_callback(gpio, handler->event_mask, TRUE, callback);
}

void irq_handler_remove_callback(irq_handler* handler, uint gpio) {
  gpio_set_irq_enabled(gpio, handler->event_mask, FALSE);
}
