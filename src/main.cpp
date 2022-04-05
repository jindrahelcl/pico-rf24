#include <stdio.h>
#include <RF24.h>
#include "pico/stdlib.h"

RF24 radio(17, 14); // CE and CSN pins (respectively)
uint8_t pipes[][6] = {"1Node", "2Node"};

bool radioNumber = 1;  // 0 uses pipes[0] to transmit, 1 uses pipes[1] to transmit
bool role = false; // whether sending or receiving. false is receiving

float payload = 3.0;

const int BUTTON_PIN = 15;
const int LED_PIN = 16;

int main() {
    // Initialize chosen serial port
    stdio_init_all();

    // wait for us to connect to the serial interface
    while (!stdio_usb_connected()) {
        sleep_ms(100);
    }

    // initialize button and LED pins
    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_down(BUTTON_PIN); // ensure it is not floating when button not pressed

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    if (!radio.begin()) {
        printf("Radio hardware is not responding!\n");
        return 1;
    }

    printf("Hello, this is vysilacka software.\n");
    printf("Which radio is this? Enter 0 or 1. Default 0\n");
    int c = getchar_timeout_us(10000000);
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

    printf("setting up radio and opening pipes.\n");
    radio.setPALevel(RF24_PA_MAX);
    radio.setPayloadSize(sizeof(payload));

    radio.openReadingPipe(1, pipes[!radioNumber]);
    radio.openWritingPipe(pipes[radioNumber]);

    printf("starting listening\n");
    radio.startListening();

    printf("entering while loop in 2 seconds\n");
    sleep_ms(2000);

    // Loop forever
    while (true) {

        if (role) {
            // transmitting...
            radio.stopListening();

            unsigned long start_timer = time_us_32();                    // start the timer
            bool report = radio.write(&payload, sizeof(float));      // transmit & save the report
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
            radio.startListening();

            uint8_t pipe;
            if (radio.available(&pipe)) {
                uint8_t bytes = radio.getPayloadSize();
                radio.read(&payload, bytes);
                printf("received %d bytes on pipe %d: %f\n", bytes, pipe, payload);

                gpio_put(LED_PIN, 1);
            }
            else {
                gpio_put(LED_PIN, 0);
            }
        }

        // change role?
        if(gpio_get(BUTTON_PIN)) {
            printf("button is pressed, setting role to transmit\n");
            role = 1;
        }
        else {
            role = 0;
        }

    }

}