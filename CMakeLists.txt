# Set minimum required version of CMake
cmake_minimum_required(VERSION 3.12)

# Include build functions from Pico SDK
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)
include($ENV{PICO_EXTRAS_PATH}/external/pico_extras_import.cmake)

# Include the RF24 library
include(lib/RF24/CMakeLists.txt)

# Set name of project (as PROJECT_NAME) and C/C++ standards
project(vysilacka C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Creates a pico-sdk subdirectory in our project for the libraries
pico_sdk_init()

# Tell CMake where to find the executable source file
add_executable(${PROJECT_NAME}
  src/main.cpp
)

add_executable(echo
  src/echo.c
  src/microphone.c
  src/microphone.h
  src/button.c
  src/button.h
)

add_executable(test_button
  src/test_button.c
  src/button.c
  src/button.h
)

add_executable(test_usb
  src/test_usb.cpp
)

add_executable(test_pwm_speaker
  src/test_pwm_speaker.cpp
)

add_executable(sine_wave
  src/sine_wave.c
)

add_executable(hello_world_microphone
  src/hello_world_microphone.c src/microphone.c
  src/microphone.h
)


# Create map/bin/hex/uf2 files
pico_add_extra_outputs(${PROJECT_NAME})
pico_add_extra_outputs(test_usb)
pico_add_extra_outputs(test_pwm_speaker)
pico_add_extra_outputs(hello_world_microphone)
pico_add_extra_outputs(test_button)
pico_add_extra_outputs(sine_wave)
pico_add_extra_outputs(echo)

# Link to pico_stdlib (gpio, time, etc. functions)
target_link_libraries(${PROJECT_NAME}
  pico_stdlib
  hardware_spi
  hardware_timer
  hardware_clocks
  hardware_pwm
  RF24
)

target_link_libraries(echo
  pico_stdlib
  hardware_timer
  pico_stdio_usb
  hardware_adc
  hardware_dma
  hardware_pwm
  pico_audio_pwm
)

target_link_libraries(test_usb
  pico_stdlib
  hardware_timer
  pico_stdio_usb
)

target_link_libraries(hello_world_microphone
  pico_stdlib
  hardware_timer
  pico_stdio_usb
  hardware_adc
  hardware_dma
)

target_link_libraries(test_button
  pico_stdlib
  hardware_gpio
  pico_stdio_usb
)


target_link_libraries(sine_wave
  pico_stdlib
  hardware_timer
  pico_stdio_usb
  hardware_adc
  hardware_dma
  pico_audio_pwm
)

target_link_libraries(test_pwm_speaker
  pico_stdlib
  hardware_timer
  hardware_clocks
  hardware_pwm
  pico_stdio_usb
)

target_compile_definitions(${PROJECT_NAME}
  PUBLIC PICO_DEFAULT_SPI=0
  PUBLIC PICO_DEFAULT_SPI_SCK_PIN=6
  PUBLIC PICO_DEFAULT_SPI_TX_PIN=7
  PUBLIC PICO_DEFAULT_SPI_RX_PIN=4
  PUBLIC RADIO_NUMBER=0
  PUBLIC INTERACTIVE=false
)

# Enable usb output, disable uart output
pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 0)
