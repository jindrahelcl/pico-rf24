/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "hardware/clocks.h"
#include "hardware/structs/clocks.h"
#include "pico/audio_pwm.h"
#include "pico/stdlib.h"

#define SINE_WAVE_TABLE_LEN 2048
#define SAMPLES_PER_BUFFER 256
#define AUDIO_PIN 10

static int16_t sine_wave_table[SINE_WAVE_TABLE_LEN];

struct audio_buffer_pool *init_audio() {
  static audio_format_t audio_format = {
    .format = AUDIO_BUFFER_FORMAT_PCM_S16,
    .sample_freq = 24000,
    .channel_count = 1,
  };

  static struct audio_buffer_format producer_format = {
    .format = &audio_format,
    .sample_stride = 2
  };

  struct audio_buffer_pool *producer_pool = audio_new_producer_pool(&producer_format, 3,
                                                                    SAMPLES_PER_BUFFER); // todo correct size
  bool __unused ok;
  const struct audio_format *output_format;

  audio_pwm_channel_config_t audio_cfg = default_mono_channel_config;
  audio_cfg.core.base_pin = AUDIO_PIN;

  output_format = audio_pwm_setup(&audio_format, -1, &audio_cfg);
  if (!output_format) {
    panic("PicoAudio: Unable to open audio device.\n");
  }
  ok = audio_pwm_default_connect(producer_pool, false);
  assert(ok);
  audio_pwm_set_enabled(true);
  return producer_pool;
}

int main() {
  set_sys_clock_48mhz();
  stdio_init_all();

  for (int i = 0; i < SINE_WAVE_TABLE_LEN; i++) {
    sine_wave_table[i] = 32767 * cosf(i * 2 * (float) (M_PI / SINE_WAVE_TABLE_LEN));
  }

  struct audio_buffer_pool *ap = init_audio();
  uint32_t step = 0x200000;
  uint32_t pos = 0;
  uint32_t pos_max = 0x10000 * SINE_WAVE_TABLE_LEN;
  uint vol = 128;
  while (true) {
    enum audio_correction_mode m = audio_pwm_get_correction_mode();
    int c = getchar_timeout_us(0);
    if (c >= 0) {
      if (c == '-' && vol) vol -= 4;
      if ((c == '=' || c == '+') && vol < 255) vol += 4;
      if (c == '[' && step > 0x10000) step -= 0x10000;
      if (c == ']' && step < (SINE_WAVE_TABLE_LEN / 16) * 0x20000) step += 0x10000;
      if (c == 'q') break;
      if (c == 'c') {
        bool done = false;
        while (!done) {
          if (m == none) m = fixed_dither;
          else if (m == fixed_dither) m = dither;
          else if (m == dither) m = noise_shaped_dither;
          else if (m == noise_shaped_dither) m = none;
          done = audio_pwm_set_correction_mode(m);
        }
      }
      printf("vol = %d, step = %d mode = %d      \r", vol, step >>16, m);
    }
    struct audio_buffer *buffer = take_audio_buffer(ap, true);
    int16_t *samples = (int16_t *) buffer->buffer->bytes;
    for (uint i = 0; i < buffer->max_sample_count; i++) {
      samples[i] = (vol * sine_wave_table[pos >> 16u]) >> 8u;
      pos += step;
      if (pos >= pos_max) pos -= pos_max;
    }
    buffer->sample_count = buffer->max_sample_count;
    give_audio_buffer(ap, buffer);
  }
  puts("\n");
  return 0;
}
