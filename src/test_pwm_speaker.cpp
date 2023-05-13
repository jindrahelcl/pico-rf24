#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/pwm.h"

const int BUTTON_PIN = 15;
const int AUDIO_PIN = 10;
const int LED_PIN = 25;

float start_freq = 100;


void get_pwm_params(const uint32_t freq, const uint16_t duty, uint32_t* top, uint16_t* level) {
    *top = 1000000UL / freq - 1;
    *level = (*top + 1) * duty / 100 - 1;
}

int main(void) {
  stdio_init_all();

  // initialize button and LED pins
  gpio_init(BUTTON_PIN);
  gpio_set_dir(BUTTON_PIN, GPIO_IN);
  gpio_pull_down(BUTTON_PIN); // ensure it is not floating when button not pressed

  gpio_init(LED_PIN);
  gpio_set_dir(LED_PIN, GPIO_OUT);

  bool playing = false;

  gpio_set_function(AUDIO_PIN, GPIO_FUNC_PWM);
  uint slice_num = pwm_gpio_to_slice_num(AUDIO_PIN);
  uint channel = pwm_gpio_to_channel(AUDIO_PIN);

  pwm_config cfg = pwm_get_default_config();
  float clock_divider = static_cast<float>(clock_get_hz(clk_sys)) / 1000000UL;

  pwm_config_set_clkdiv(&cfg, clock_divider);
  pwm_config_set_clkdiv_mode(&cfg, PWM_DIV_FREE_RUNNING);
  pwm_config_set_phase_correct(&cfg, true);
  pwm_init(slice_num, &cfg, false);

  float freq = 200;

  // Loop forever
  while (true) {

    if(gpio_get(BUTTON_PIN)) {

      if(!playing) {
        printf("pressed\n");
        playing = true;
        pwm_set_enabled(slice_num, true);
        gpio_put(LED_PIN, 1);
      }
      freq += 1;

      uint32_t top;
      uint16_t level;

      get_pwm_params(freq, 50, &top, &level);
      printf("sending frequency %.2f\r", freq);

      pwm_set_wrap(slice_num, top);
      pwm_set_chan_level(slice_num, channel, level);

    }
    else {
      if(playing) {
        printf("\nreleased\n");
        freq = start_freq;
        playing = false;
        pwm_set_enabled(slice_num, false);
        gpio_put(LED_PIN, 0);
      }
    }
  }

  return 0;
}
