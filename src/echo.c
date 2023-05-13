#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/audio_pwm.h"
#include "hardware/clocks.h"

#include "button.h"
#include "microphone.h"

#define SAMPLE_BUFSIZE 256
#define MEMORY_SIZE 16000
#define AUDIO_PIN 10

const int BUTTON_PIN = 15;
const int LED_PIN = 25;

typedef enum {
  PLAYING,
  RECORDING,
  IDLE
} state;

state st = IDLE;
struct audio_buffer_pool *ap;

// configuration
analog_microphone_config config = {
  // GPIO to use for input, must be ADC compatible (GPIO 26 - 28)
  .gpio = 26,

  // bias voltage of microphone in volts
  .bias_voltage = 1.69,

  // sample rate in Hz
  .sample_rate = 8000,

  // number of samples to buffer
  .sample_buffer_size = SAMPLE_BUFSIZE,
};

//int_t bias = ((int16_t)((config.bias_voltage * 4095) / 3.3));
int16_t sample_buffer[SAMPLE_BUFSIZE];
volatile int samples_read = 0;

int16_t memory[MEMORY_SIZE];
int memory_offset = 0;


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
                                                                    256); // todo correct size
  bool __unused ok;
  const struct audio_format *output_format;

  audio_pwm_channel_config_t audio_cfg = default_mono_channel_config;
  audio_cfg.core.base_pin = AUDIO_PIN;

  printf("setting up pwm");
  output_format = audio_pwm_setup(&audio_format, -1, &audio_cfg);
  printf(" DONE\n");

  if (!output_format) {
    panic("PicoAudio: Unable to open audio device.\n");
  }
  ok = audio_pwm_default_connect(producer_pool, false);
  assert(ok);
  audio_pwm_set_enabled(true);
  return producer_pool;
}


void on_analog_samples_ready()
{
  // callback from library when all the samples in the library
  // internal sample buffer are ready for reading
  samples_read = analog_microphone_read(sample_buffer, SAMPLE_BUFSIZE);
  printf("im here");
  // do we have free memory?
  if(memory_offset + samples_read < MEMORY_SIZE) {
    memcpy(memory + memory_offset, sample_buffer, sizeof(int16_t) * samples_read);
    memory_offset += samples_read;
  }
  else {
    // indicate that we are not saving anymore
    gpio_put(LED_PIN, 0);
  }
}

void start_recording() {
  // start capturing data from the analog microphone
  gpio_put(LED_PIN, 1);
  if (analog_microphone_start() < 0) {
    printf("PDM microphone start failed!\n");
    while (1) { tight_loop_contents();  }
  }
}

void stop_recording() {
  gpio_put(LED_PIN, 0);
  printf("stopping mic");
  analog_microphone_stop();
  printf(" done\n");
}

void play_buffer() {

  printf("taking buffer\n");
  audio_buffer_t *buffer = take_audio_buffer(ap, true);

  // i have a memory array memory[i]and I have memory_offset, which marks the end

  printf("playing from memory\n");
  uint read_offset = 0;
  while(read_offset < memory_offset) {
    int16_t *samples = (int16_t *) buffer->buffer->bytes;

    for (uint i = 0; i < buffer->max_sample_count; i++) {
      printf("playing %d\n", memory[read_offset++]);
      samples[i] = memory[read_offset];
      if(read_offset >= memory_offset)
        break;
    }

    buffer->sample_count = buffer->max_sample_count;
    give_audio_buffer(ap, buffer);
  }

  /* if(memory_offset > 5) { */
  /*   printf("I have %d samples in memory right now, here are the first five.\n", memory_offset); */
  /*   for(int i = 0; i < 5; i++) { */
  /*     printf("%d", memory[i]); */
  /*   } */
  /*   printf("\n"); */
  /* } */

  printf("resetting\n");

  memory_offset = 0;
}

void onchange(button_t* button_p) {
  button_t *button = (button_t*)button_p;
  printf("Button on pin %d changed its state to %d\n", button->pin, button->state);

  printf("onchange!\n");
  printf("assa\n");
  if(button->state) {
    printf("button is pressed!\n");
    // push button - run mic
    switch(st){
      case RECORDING:
        // keep recording
        break;

      case IDLE:
        // start recording
        st = RECORDING;
        start_recording();
        break;
    }
  }
  else {
    printf("button release\n");
    // release button - replay to speaker
    switch(st){
      case RECORDING:
        // stop recording, play sound
        stop_recording();
        play_buffer();
        st = IDLE;
        break;

      case IDLE:
        // nothing happens
        break;
    }
  }
}



int main(void) {
  set_sys_clock_48mhz();
  stdio_init_all();

  gpio_init(LED_PIN);
  gpio_set_dir(LED_PIN, GPIO_OUT);

  while (!stdio_usb_connected()) {
    tight_loop_contents();
  }
  ap = init_audio();
  printf("audio initialized!\n");

  // initialize the analog microphone
  if (analog_microphone_init(&config) < 0) {
    printf("analog microphone initialization failed!\n");
    while (1) { tight_loop_contents(); }
  }

  // set callback that is called when all the samples in the library
  // internal sample buffer are ready for reading
  //analog_microphone_set_samples_ready_handler(stream_completed_samples);
  analog_microphone_set_samples_ready_handler(on_analog_samples_ready);
  printf("microphone initialized!\n");

  create_button(BUTTON_PIN, onchange);
  printf("button initialized!\n");

  while(1)
    tight_loop_contents();
}
