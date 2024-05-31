cmake_minimum_required(VERSION 3.10)

set(CMAKE_C_COMPILER /usr/bin/avr-gcc)
set(CMAKE_CXX_COMPILER /usr/bin/avr-g++)
#set(CMAKE_ASM_COMPILER  /usr/bin/avr-as)

project(DS2502 LANGUAGES C CXX ASM)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_BUILD_TYPE MinSizeRel)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -mmcu=atmega328p -Wno-comment -Wno-volatile -DF_CPU=16000000L -DARDUINO=10819 -DARDUINO_AVR_UNO -DARDUINO_ARCH_AVR -Wall")
set(CMAKE_C_FLAGS  "${CMAKE_C_FLAGS} -mmcu=atmega328p -DF_CPU=16000000L -DARDUINO=10819 -DARDUINO_AVR_UNO -DARDUINO_ARCH_AVR")
set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} -mmcu=atmega328p")

include_directories("${CMAKE_SOURCE_DIR}/src/arduino")
include_directories("${CMAKE_SOURCE_DIR}/src/libs")

# Define a variable containing a list of source files for the project
set(SRC_FILES
        src/main.cpp
        src/libs/OneWire/OneWire.cpp
)

# Define the build target for the executable
add_executable(${PROJECT_NAME}
        ${SRC_FILES} src/arduino/wiring_pulse.S src/arduino/hooks.c src/arduino/WInterrupts.c src/arduino/wiring.c src/arduino/wiring_analog.c src/arduino/wiring_digital.c src/arduino/wiring_pulse.c src/arduino/wiring_shift.c src/arduino/abi.cpp src/arduino/CDC.cpp src/arduino/HardwareSerial.cpp src/arduino/HardwareSerial0.cpp src/arduino/HardwareSerial1.cpp src/arduino/HardwareSerial2.cpp src/arduino/HardwareSerial3.cpp src/arduino/IPAddress.cpp src/arduino/new.cpp src/arduino/PluggableUSB.cpp src/arduino/Print.cpp src/arduino/Stream.cpp src/arduino/Tone.cpp src/arduino/USBCore.cpp src/arduino/WMath.cpp src/arduino/WString.cpp )

set_property(TARGET ${CMAKE_PROJECT_NAME} PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)

add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
        #COMMAND avr-objcopy -j .text -j .data -O ihex ${CMAKE_PROJECT_NAME} ${CMAKE_PROJECT_NAME}.hex && avrdude -p m328p -c stk500v2 -P /dev/ttyACM0 -b 115200 -F -D -U flash:w:${CMAKE_PROJECT_NAME}.hex && exit 0
        #COMMAND avr-objcopy -j .text -j .data -O ihex arduinoTest arduinoTest.hex && avrdude -p m2560 -c stk500v1 -P /dev/ttyACM0 -b 19200 -F -D -U flash:w:arduinoTest.hex && exit 0
        COMMAND avr-objcopy -O ihex -R .eeprom ${CMAKE_PROJECT_NAME} ${CMAKE_PROJECT_NAME}.hex && avrdude -v -patmega328p -carduino -P/dev/ttyACM0 -b115200 -D -Uflash:w:${CMAKE_PROJECT_NAME}.hex:i
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Uploading ..."
        )
