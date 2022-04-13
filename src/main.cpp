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

void get_pwm_params(const uint32_t freq, const uint16_t duty, uint32_t* top, uint16_t* level) {
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
    uint pwm_channel = pwm_gpio_to_channel(AUDIO_PIN);

    pwm_config cfg = pwm_get_default_config();
    float clock_divider = static_cast<float>(clock_get_hz(clk_sys)) / 1000000UL;

    pwm_config_set_clkdiv(&cfg, clock_divider);
    pwm_config_set_clkdiv_mode(&cfg, PWM_DIV_FREE_RUNNING);
    pwm_config_set_phase_correct(&cfg, true);
    pwm_init(slice_num, &cfg, false);

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

    float tx_freq = start_freq; // this is the frequency to transmit
    float rx_freq; // this is the frequency we are reading

    // Loop forever
    while (true) {

        // update role
        if(gpio_get(BUTTON_PIN)) {
            // button is pressed, transmit

            if (!transmitting) {
                // we were not transmiting last time, so commence transmitting
                printf("button is pressed, setting role to transmit\n");
                radio.stopListening();

                // disable speaker, reset frequency
                pwm_set_enabled(slice_num, false);
                tx_freq = start_freq;
            }

            transmitting = 1;
        }
        else {
            // button is not pressed, listen

            if (transmitting) {
                // we were transmitting, so reset everything.
                printf("button released, listening again.\n");
                radio.startListening();
            }

            transmitting = 0;
        }

        if (transmitting) {
            // we are transmitting
            unsigned long start_timer = time_us_32();                    // start the timer
            bool report = radio.write(&tx_freq, sizeof(float));      // transmit & save the report
            unsigned long end_timer = time_us_32();                      // end the timer

            // here we are not checking the ack, so just increase the frequency.
            tx_freq += 1;

        }
        else {
            // receiving...
            uint8_t pipe;
            if (radio.available(&pipe)) {
                uint8_t bytes = radio.getPayloadSize();
                radio.read(&rx_freq, bytes);
                printf("received %d bytes on pipe %d: %f\n", bytes, pipe, rx_freq);

                gpio_put(LED_PIN, 1);

                uint32_t top;
                uint16_t level;
                get_pwm_params(rx_freq, 50, &top, &level);
                printf("setting PWM params to TOP %d, and LEVEL %d", top, level);

                pwm_set_wrap(slice_num, top);
                pwm_set_chan_level(slice_num, pwm_channel, level);
                pwm_set_enabled(slice_num, true);
            }
            else {
                gpio_put(LED_PIN, 0);
                pwm_set_enabled(slice_num, false);
            }
        }
    }
}