/*
 * Copyright (c) 2021 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 *
 * This examples captures data from an analog microphone using a sample
 * rate of 8 kHz and prints the sample values over the USB serial
 * connection.
 */

#include <stdio.h>

#include "pico/stdlib.h"
#include "microphone.h"

#define SAMPLE_BUFSIZE 512

// configuration
const struct analog_microphone_config config = {
    // GPIO to use for input, must be ADC compatible (GPIO 26 - 28)
    .gpio = 26,

    // bias voltage of microphone in volts
    .bias_voltage = 1.69,

    // sample rate in Hz
    .sample_rate = 1600, // USB is capable of getting 8000 lines in 5 seconds

    // number of samples to buffer
    .sample_buffer_size = SAMPLE_BUFSIZE,
};

int16_t bias = ((int16_t)((config.bias_voltage * 4095) / 3.3));

// variables
int16_t sample_buffer[SAMPLE_BUFSIZE];
volatile int samples_read = 0;

void on_analog_samples_ready()
{
    // callback from library when all the samples in the library
    // internal sample buffer are ready for reading
    samples_read = analog_microphone_read(sample_buffer, SAMPLE_BUFSIZE);
}

void stream_completed_samples() {
  uint16_t* buffer = analog_microphone_completed_buffer();

  /* printf("%i\n", buffer); */
  /* printf("got it"); */

  for(int i = 0; i < 100; i++) {
    printf("%i\n", buffer[i]);
  }
}

int main( void )
{
    // initialize stdio and wait for USB CDC connect
    stdio_init_all();

    while (!stdio_usb_connected()) {
       tight_loop_contents();
    }

    printf("hello analog microphone\n");

    // initialize the analog microphone
    if (analog_microphone_init(&config) < 0) {
        printf("analog microphone initialization failed!\n");
        while (1) { tight_loop_contents(); }
    }

    // set callback that is called when all the samples in the library
    // internal sample buffer are ready for reading
    //analog_microphone_set_samples_ready_handler(stream_completed_samples);
    analog_microphone_set_samples_ready_handler(on_analog_samples_ready);

    // start capturing data from the analog microphone
    if (analog_microphone_start() < 0) {
        printf("PDM microphone start failed!\n");
        while (1) { tight_loop_contents();  }
    }

    while (1) {
      tight_loop_contents();
        // wait for new samples
        while (samples_read == 0) { tight_loop_contents(); }

        // store and clear the samples read from the callback
        int sample_count = samples_read;
        samples_read = 0;

        // loop through any new collected samples
        for (int i = 0; i < sample_count; i++) {
            printf("%i\n", sample_buffer[i]);
        }
    }

    return 0;
}
