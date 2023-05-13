/*
 * Copyright (c) 2021 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hardware/adc.h"
#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "hardware/irq.h"

#include "microphone.h"

#define ANALOG_RAW_BUFFER_COUNT 4

static struct {
  analog_microphone_config config;
  int dma_channel;
  uint16_t* raw_buffer[ANALOG_RAW_BUFFER_COUNT];
  volatile int raw_buffer_write_index;  // these two vars can be changed
  volatile int raw_buffer_read_index;   // at any point from the outside.
  uint buffer_size;
  int16_t bias;
  uint dma_irq;
  analog_samples_ready_handler_t samples_ready_handler;
} analog_mic;

static void analog_dma_handler();

int analog_microphone_init(analog_microphone_config* config) {
  // zero out the memory allocated for analog mic struct
  memset(&analog_mic, 0x00, sizeof(analog_mic));

  // copy the config struct
  memcpy(&analog_mic.config, config, sizeof(analog_mic.config));

  // must use an ADC pin (26--29)
  if (config->gpio < 26 || config->gpio > 29) {
    return -1;
  }

  // size of one buffer in bytes (2 bytes per item)
  size_t raw_buffer_size =
      config->sample_buffer_size * sizeof(analog_mic.raw_buffer[0][0]);
  analog_mic.buffer_size = config->sample_buffer_size;


  analog_mic.bias = ((int16_t)((config->bias_voltage * 4095) / 3.3));

  // initialize buffers
  for (int i = 0; i < ANALOG_RAW_BUFFER_COUNT; i++) {
    analog_mic.raw_buffer[i] = malloc(raw_buffer_size);
    if (analog_mic.raw_buffer[i] == NULL) {
      // failed to allocate memory for buffer, abort everything.
      analog_microphone_deinit();
      return -1;
    }
  }

  // setting up DMA
  // there are 12 channels, we just use one that is not in use.
  analog_mic.dma_channel = dma_claim_unused_channel(true);

  if (analog_mic.dma_channel < 0) {
    // failed to allocate a free DMA channel
    analog_microphone_deinit();
    return -1;
  }


  // get the default DMA config struct
  dma_channel_config dma_channel_cfg = dma_channel_get_default_config(
      analog_mic.dma_channel);

  // we set the size of the data item to 2 bytes (output from ADC has 2 bytes)
  channel_config_set_transfer_data_size(&dma_channel_cfg, DMA_SIZE_16);

  // no read increment - we don't read from an addresed space
  channel_config_set_read_increment(&dma_channel_cfg, false);

  // we want write increment - we are writing to memory so each time we write
  // we want to move to the next position in mem
  channel_config_set_write_increment(&dma_channel_cfg, true);

  // ADC will be in control of the transfer pacing
  channel_config_set_dreq(&dma_channel_cfg, DREQ_ADC);

  // which IRQ channel to use (when the buffer is full)
  analog_mic.dma_irq = DMA_IRQ_0;

  // now we have the config struct ready, so let's initialize the thing!
  dma_channel_configure(
      analog_mic.dma_channel,    // unused channel
      &dma_channel_cfg,          // config object
      analog_mic.raw_buffer[0],  // beginning of the first buffer (WRITE addr)
      &adc_hw->fifo,             // READ addr (the ADC's FIFO)
      analog_mic.buffer_size,    // how many transfers
      false);                    // do not start immediately

  // next, configure the ADC hardware
  adc_gpio_init(config->gpio);

  adc_init();
  adc_select_input(config->gpio - 26);
  adc_fifo_setup(
      true,    // Write each completed conversion to the sample FIFO
      true,    // Enable DMA data request (DREQ)
      1,       // DREQ (and IRQ) asserted when at least 1 sample present
      false,   // We won't see the ERR bit because of 8 bit reads; disable.
      false);  // Don't shift each sample to 8 bits when pushing to FIFO

  // clock divider. the ADC works at 48MHz. we divide that by sample rate. For
  // 8kHz that gives us 6000. Since the docs state the period of samples is
  // (1+div) cycles, it means that we get a reading every 6000 cycles, which
  // gives a 8kHz sampling rate.
  float clk_div = (clock_get_hz(clk_adc) / (1.0 * config->sample_rate)) - 1;
  adc_set_clkdiv(clk_div);
}

void analog_microphone_deinit() {
  // destroy the microphone struct

  for (int i = 0; i < ANALOG_RAW_BUFFER_COUNT; i++) {
    if (analog_mic.raw_buffer[i]) {
      // for each buffer we malloc'd, we free it.
      free(analog_mic.raw_buffer[i]);
      analog_mic.raw_buffer[i] = NULL;
    }
  }

  if (analog_mic.dma_channel > -1) {
    // also release the DMA channel
    dma_channel_unclaim(analog_mic.dma_channel);
    analog_mic.dma_channel = -1;
  }
}

int analog_microphone_start() {
  // start the capture!

  // first, set up the interrupt handling
  irq_set_enabled(analog_mic.dma_irq, true);
  irq_set_exclusive_handler(analog_mic.dma_irq, analog_dma_handler);

  // this is weird stuff, i think that analog_mic.dma_irq is ALWAYS DMA_IRQ_0
  if (analog_mic.dma_irq == DMA_IRQ_0) {
    dma_channel_set_irq0_enabled(analog_mic.dma_channel, true);
  } else if (analog_mic.dma_irq == DMA_IRQ_1) {
    dma_channel_set_irq1_enabled(analog_mic.dma_channel, true);
  } else {
    return -1;
  }  // anyway, this just tells the DMA to enable the irq as well.

  // reset the data pointers
  analog_mic.raw_buffer_write_index = 0;
  analog_mic.raw_buffer_read_index = 0;

  // start the DMA transfer. unclear why we again specify the config values
  // from the analog_microphone_init() function.
  dma_channel_transfer_to_buffer_now(
      analog_mic.dma_channel,
      analog_mic.raw_buffer[0],
      analog_mic.buffer_size);

  adc_run(true); // start running the adc
}

void analog_microphone_stop() {
  // basically a reverse of the start function.
  adc_run(false); // stop running the adc

  // stop DMA
  dma_channel_abort(analog_mic.dma_channel);

  // disable IRQ on DMA
  if (analog_mic.dma_irq == DMA_IRQ_0) {
    dma_channel_set_irq0_enabled(analog_mic.dma_channel, false);
  } else if (analog_mic.dma_irq == DMA_IRQ_1) {
    dma_channel_set_irq1_enabled(analog_mic.dma_channel, false);
  }

  // disable the IRQ
  irq_set_enabled(analog_mic.dma_irq, false);
}

static void analog_dma_handler() {
  // this is function is called when the buffer is full.

  // clear IRQ by directly accessing the registers (ints0, ints1).
  // these are 4-byte figures, bits 15:0 (rightmost)
  if (analog_mic.dma_irq == DMA_IRQ_0) {
    dma_hw->ints0 = (1u << analog_mic.dma_channel);
  } else if (analog_mic.dma_irq == DMA_IRQ_1) {
    dma_hw->ints1 = (1u << analog_mic.dma_channel);
  }

  // set the index of the completed buffer as the one which is supposed to be
  // read from
  analog_mic.raw_buffer_read_index = analog_mic.raw_buffer_write_index;

  // get the next capture index to send the dma to start
  analog_mic.raw_buffer_write_index =
      (analog_mic.raw_buffer_write_index + 1) % ANALOG_RAW_BUFFER_COUNT;

  // give the channel a new buffer to write to and re-trigger it
  dma_channel_transfer_to_buffer_now(
      analog_mic.dma_channel,
      analog_mic.raw_buffer[analog_mic.raw_buffer_write_index],
      analog_mic.buffer_size);

  // fire up the event which takes care of the bufferful of samples
  if (analog_mic.samples_ready_handler) {
    analog_mic.samples_ready_handler();
  }
}

void analog_microphone_set_samples_ready_handler(
    analog_samples_ready_handler_t handler) {
  analog_mic.samples_ready_handler = handler;
}

int analog_microphone_read(int16_t* buffer, size_t samples) {
  // this function is supposed to be called from the samples ready handler

  // want more samples? too bad.
  if (samples > analog_mic.config.sample_buffer_size) {
    samples = analog_mic.config.sample_buffer_size;
  }

  // if we are writing to this buffer as we speak, do nothing.
  if (analog_mic.raw_buffer_write_index == analog_mic.raw_buffer_read_index) {
    return 0;
  }

  // pointer to start of the reading buffer (the completed one)
  uint16_t* in = analog_mic.raw_buffer[analog_mic.raw_buffer_read_index];

  // we got this pointer from the function args.
  int16_t* out = buffer;

  // bias voltage (converted to the ADC output value)
  int16_t bias = analog_mic.bias;

  // next time we will read from the next buffer. questionable command, it has
  // been set in the buffer-full handler, also this can rotate.
  analog_mic.raw_buffer_read_index++;

  // basically memcpy, but subtracting bias while we're at it.
  for (int i = 0; i < samples; i++) {
    *out++ = *in++ - bias;
  }

  // return how many samples has been read.
  return samples;
}

uint16_t* analog_microphone_completed_buffer() {
  // this function does not increment the buffer_read_index, just returns a
  // pointer to the relevant buffer.
  return analog_mic.raw_buffer[analog_mic.raw_buffer_read_index];
}
