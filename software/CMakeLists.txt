cmake_minimum_required(VERSION 3.10)

set(CMAKE_C_COMPILER /usr/bin/avr-gcc)
set(CMAKE_CXX_COMPILER /usr/bin/avr-g++)
#set(CMAKE_ASM_COMPILER  /usr/bin/avr-as)

project(DS2502 LANGUAGES C CXX ASM)

#Disable the automatic lto flags
# set(CMAKE_CXX_COMPILE_OPTIONS_IPO "")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_BUILD_TYPE MinSizeRel)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(BOTH_FLAGS "-fpermissive -fno-exceptions -ffunction-sections -fdata-sections -fno-threadsafe-statics -w")
set(BOTH_FLAGS "${BOTH_FLAGS}")
set(BOTH_FLAGS "${BOTH_FLAGS} -DCMAKE")
set(BOTH_FLAGS "${BOTH_FLAGS} -mmcu=atmega328p -DF_CPU=16000000L -DARDUINO=10819 -DARDUINO_AVR_UNO -DARDUINO_ARCH_AVR")
set(CMAKE_CXX_FLAGS "${BOTH_FLAGS} ${CMAKE_CXX_FLAGS} -Wno-comment -Wno-volatile -Wall")
set(CMAKE_C_FLAGS "${BOTH_FLAGS} ${CMAKE_C_FLAGS}")
set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} -mmcu=atmega328p")

include_directories("${CMAKE_SOURCE_DIR}/src/libs/ArduinoCore-avr/cores/arduino")
include_directories("${CMAKE_SOURCE_DIR}/src/libs/ArduinoCore-avr/variants/standard")
include_directories("${CMAKE_SOURCE_DIR}/src/libs")

# Define a variable containing a list of source files for the project
set(SRC_FILES
        src/main.cpp 
        # src/test.cpp
        src/libs/OneWire/OneWire.cpp
)

# Define the build target for the executable
add_executable(${PROJECT_NAME}
        ${SRC_FILES} src/libs/ArduinoCore-avr/cores/arduino/main.cpp src/libs/ArduinoCore-avr/cores/arduino/wiring_pulse.S src/libs/ArduinoCore-avr/cores/arduino/hooks.c src/libs/ArduinoCore-avr/cores/arduino/WInterrupts.c src/libs/ArduinoCore-avr/cores/arduino/wiring.c src/libs/ArduinoCore-avr/cores/arduino/wiring_analog.c src/libs/ArduinoCore-avr/cores/arduino/wiring_digital.c src/libs/ArduinoCore-avr/cores/arduino/wiring_pulse.c src/libs/ArduinoCore-avr/cores/arduino/wiring_shift.c src/libs/ArduinoCore-avr/cores/arduino/abi.cpp src/libs/ArduinoCore-avr/cores/arduino/CDC.cpp src/libs/ArduinoCore-avr/cores/arduino/HardwareSerial.cpp src/libs/ArduinoCore-avr/cores/arduino/HardwareSerial0.cpp src/libs/ArduinoCore-avr/cores/arduino/HardwareSerial1.cpp src/libs/ArduinoCore-avr/cores/arduino/HardwareSerial2.cpp src/libs/ArduinoCore-avr/cores/arduino/HardwareSerial3.cpp src/libs/ArduinoCore-avr/cores/arduino/IPAddress.cpp src/libs/ArduinoCore-avr/cores/arduino/new.cpp src/libs/ArduinoCore-avr/cores/arduino/PluggableUSB.cpp src/libs/ArduinoCore-avr/cores/arduino/Print.cpp src/libs/ArduinoCore-avr/cores/arduino/Stream.cpp src/libs/ArduinoCore-avr/cores/arduino/Tone.cpp src/libs/ArduinoCore-avr/cores/arduino/USBCore.cpp src/libs/ArduinoCore-avr/cores/arduino/WMath.cpp src/libs/ArduinoCore-avr/cores/arduino/WString.cpp)

# get_property(allsrcfiles TARGET "${CMAKE_PROJECT_NAME}" PROPERTY SOURCES)
# foreach(in IN LISTS allsrcfiles)    
#         # string(REGEX REPLACE "(?\:[^/\\.]+/)+([^/]+)" "\\1" srcfile srcfile)
#         string(REPLACE "/" " " file "${in}")
#         separate_arguments(file)
#         list(LENGTH file len)
#         math(EXPR len "${len} - 1")
#         list(GET file ${len} file)
#         # message("${file}")
#         set_source_files_properties(${in} PROPERTIES COMPILE_FLAGS "-Wa,-aghls=${file}.lst")
# endforeach()

#LTO?
set_property(TARGET ${CMAKE_PROJECT_NAME} PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)

add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
        #COMMAND avr-objcopy -j .text -j .data -O ihex ${CMAKE_PROJECT_NAME} ${CMAKE_PROJECT_NAME}.hex && avrdude -p m328p -c stk500v2 -P /dev/ttyACM0 -b 115200 -F -D -U flash:w:${CMAKE_PROJECT_NAME}.hex && exit 0
        #COMMAND avr-objcopy -j .text -j .data -O ihex arduinoTest arduinoTest.hex && avrdude -p m2560 -c stk500v1 -P /dev/ttyACM0 -b 19200 -F -D -U flash:w:arduinoTest.hex && exit 0
        COMMAND avr-objcopy -O ihex -R .eeprom ${CMAKE_PROJECT_NAME} ${CMAKE_PROJECT_NAME}.hex && avrdude -v -patmega328p -carduino -P/dev/ttyACM0 -b115200 -D -Uflash:w:${CMAKE_PROJECT_NAME}.hex:i
        # COMMAND avr-objcopy -O ihex -R .eeprom ${CMAKE_PROJECT_NAME} ${CMAKE_PROJECT_NAME}.hex
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Uploading ..." 
        )

