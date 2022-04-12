#include <stdio.h>
#include <RF24.h>
#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/pwm.h"

RF24 radio(17, 14); // CE and CSN pins (respectively)
uint8_t pipes[][6] = {"1Node", "2Node"};

bool radioNumber = RADIO_NUMBER;  // 0 uses pipes[0] to transmit, 1 uses pipes[1] to transmit
bool transmitting = false; // whether sending or receiving. false is receiving

float start_freq = 100;

const int BUTTON_PIN = 15;
const int LED_PIN = 16;
const int AUDIO_PIN = 10;

void get_pwm_params(const uint32_t freq, const uint16_t duty, uint32_t* top, uint16_t* level, float* divider) {
    uint32_t f_sys = clock_get_hz(clk_sys);
    *divider = f_sys / 1000000UL;
    *top = 1000000UL / freq - 1;
    *level = (*top + 1) * duty / 100 - 1;
}

int main() {
    // Initialize chosen serial port
    stdio_init_all();

    // wait for us to connect to the serial interface
    #if INTERACTIVE
    while (!stdio_usb_connected()) {
        sleep_ms(100);
    }
    #endif

    // initialize button and LED pins
    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_down(BUTTON_PIN); // ensure it is not floating when button not pressed

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    gpio_set_function(AUDIO_PIN, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(AUDIO_PIN);

    if (!radio.begin()) {
        printf("Radio hardware is not responding!\n");
        return 1;
    }

    printf("Hello, this is vysilacka software.\n");

    #if INTERACTIVE
    printf("Which radio is this? Enter 0 or 1. Default 0\n");
    int c = getchar_timeout_us(20000000);
    printf("You said: %c\n", c);
    switch(c) {
        case '0':
            radioNumber = 0;
            break;
        case '1':
            radioNumber = 1;
            break;
        case PICO_ERROR_TIMEOUT:
            printf("zmrde, vytimeoutoval jsi\n");
            radioNumber = 0;
            break;
        default:
            printf("You did not say 0 or 1. Settting to default 0.\n");
            radioNumber = 0;
            break;
    }
    #endif

    printf("setting up radio and opening pipes.\n");
    radio.setPALevel(RF24_PA_MAX);
    radio.setPayloadSize(sizeof(float));
    radio.setAutoAck(false);

    radio.openReadingPipe(1, pipes[!radioNumber]);
    radio.openWritingPipe(pipes[radioNumber]);

    printf("starting listening\n");
    radio.startListening();

    printf("entering while loop in 2 seconds\n");

    #if INTERACTIVE
    sleep_ms(2000);
    #endif

    float freq = start_freq;

    // Loop forever
    while (true) {

        if (transmitting) {
            // transmitting...
            unsigned long start_timer = time_us_32();                    // start the timer
            bool report = radio.write(&freq, sizeof(float));      // transmit & save the report
            unsigned long end_timer = time_us_32();                      // end the timer

            if (report) {
                printf("Transmission successful, time to transmit: %d us\n", end_timer - start_timer);
            }
            else {
                printf("Transmission failed or timed out.\n");
            }

            //sleep_ms(500);

        }
        else {
            // receiving...
            uint8_t pipe;
            if (radio.available(&pipe)) {
                uint8_t bytes = radio.getPayloadSize();
                radio.read(&freq, bytes);
                printf("received %d bytes on pipe %d: %f\n", bytes, pipe, freq);

                gpio_put(LED_PIN, 1);

                uint32_t top;
                uint16_t level;
                float divider;

                printf("setting PWM params to TOP %d, DIVIDER %.2f, and LEVEL %d", top, divider, level);

                get_pwm_params(freq, 50, &top, &level, &divider);

                pwm_set_wrap(slice_num, top);
                pwm_set_chan_level(slice_num, AUDIO_PIN, level);
                pwm_set_enabled(slice_num, true);
            }
            else {
                gpio_put(LED_PIN, 0);
            }
        }

        // change role?
        if(gpio_get(BUTTON_PIN)) {
            if (!transmitting) {
                printf("button is pressed, setting role to transmit\n");
                radio.stopListening();
                freq = start_freq;
            }
            else {
                freq += 1;
            }

            transmitting = 1;

        }
        else {
            if (transmitting) {
                printf("button released, listening again.\n");
                radio.startListening();
                freq = start_freq;

                pwm_set_enabled(slice_num, false);
            }
            transmitting = 0;
        }

    }

}