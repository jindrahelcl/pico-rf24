/*
 * Copyright (c) 2021 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef _PICO_ANALOG_MICROPHONE_H_
#define _PICO_ANALOG_MICROPHONE_H_

typedef void (*analog_samples_ready_handler_t)(void);

typedef struct {
    uint gpio;
    float bias_voltage;
    uint sample_rate;
    uint sample_buffer_size;
} analog_microphone_config;

int analog_microphone_init(analog_microphone_config* config);
void analog_microphone_deinit();

int analog_microphone_start();
void analog_microphone_stop();

void analog_microphone_set_samples_ready_handler(analog_samples_ready_handler_t handler);

int analog_microphone_read(int16_t* buffer, size_t samples);
uint16_t* analog_microphone_completed_buffer();

#endif
