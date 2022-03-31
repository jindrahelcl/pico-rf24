#include <stdio.h>
#include <RF24.h>
#include "pico/stdlib.h"

RF24 radio(7, 8); // CE and CSN pins (respectively)
uint8_t pipes[][6] = {"Node1", "Node2"};

bool radioNumber = 1;  // 0 uses pipes[0] to transmit, 1 uses pipes[1] to transmit
bool role = false; // whether sending or receiving. false is receiving

float payload = 3.0;

const int BUTTON_PIN = 15;
const int LED_PIN = 16;

int main() {
    // Initialize chosen serial port
    stdio_init_all();

    // initialize button and LED pins
    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_down(BUTTON_PIN); // ensure it is not floating when button not pressed

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    gpio_put(LED_PIN, 1);
    delay(500);
    gpio_put(LED_PIN, 0);
    delay(100);

    if (!radio.begin()) {
        gpio_put(LED_PIN, 1);
        printf("Radio hardware is not responding!\n");
        while (true) {}
    }

    gpio_put(LED_PIN, 1);
    delay(400);
    gpio_put(LED_PIN, 0);
    delay(100);
    gpio_put(LED_PIN, 1);
    delay(400);
    gpio_put(LED_PIN, 0);

    printf("Hello, this is vysilacka software.\n");
    printf("Which radio is this? Enter 0 or 1. Default 0\n");
    char c = getchar();
    printf("You said: %c", c);
    switch(c) {
        case '0':
            radioNumber = 0;
            break;
        case '1':
            radioNumber = 1;
            break;
        default:
            printf("You did not say 0 or 1. Bye.");
            return 1;
    }

    radio.setPALevel(RF24_PA_MAX);
    radio.setPayloadSize(sizeof(payload));

    radio.openReadingPipe(1, pipes[!radioNumber]);
    radio.openWritingPipe(pipes[radioNumber]);

    if(role) {
        radio.stopListening();
    }
    else {
        radio.startListening();
    }

    // Loop forever
    while (true) {


        if (role) {
            // transmitting...
            unsigned long start_timer = time_us_32();                    // start the timer
            bool report = radio.write(&payload, sizeof(float));      // transmit & save the report
            unsigned long end_timer = time_us_32();                      // end the timer

            if (report) {
                printf("Transmission successful, time to transmit: %d us", end_timer - start_timer);
            }
            else {
                printf("Transmission failed or timed out.");
            }

            delay(500);
        }
        else {
            // receiving...

            uint8_t pipe;
            if (radio.available(&pipe)) {
                uint8_t bytes = radio.getPayloadSize();
                radio.read(&payload, bytes);
                printf("received %d bytes on pipe %d: %f", bytes, pipe, payload);
            }
        }

        // change role?
        if(gpio_get(BUTTON_PIN)) {
            printf("button is pressed, setting role to transmit");
            role = 1;
        }
        else {
            role = 0;
        }

    }

}