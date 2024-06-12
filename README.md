# Arduino based DS2502 controller

The DS2502 is a 1024bit add-only EPROM. You can add data but cannot change it. It is used mainly for device identification purposes like serial number, manufacturer data, unique identifiers, Dell Laptop Power supplies, etc. It uses the Maxim 1-wire bus. This repository is kind of like a multitool for DS2502s. It lets you read, write, check the status, and check the ID of these chips and all with a (fairly) intuitive interface. If you have any issues or questions feel free to make a GitHub issue.

When cloning this repository make sure to also clone the submodules by using `git clone --recursive`. This repository has a submodule for the OneWire library and one for the Arduino core. Both are for the CMake build system.

## Software
There are two ways to build the software. The first one is as a normal Arduino sketch. To use it open `software/src/src.ino` in the Arduino IDE and upload. Make sure to have the [OneWire library](https://www.arduino.cc/reference/en/libraries/onewire/) installed. The second way is with CMake. This method was made to allow the software to be written in IDEs other than the Arduino IDE. It probably won't build on platforms other than Linux and it definitely won't work for anything other than a Arduino Uno, but could be adapted easily to fix either of those issues.

The software will only work with LF line endings. It will not work with CR or CRLF line endings. This is the default in the Arduino IDE serial monitor, but in other programs it is not. To connect using picocom for example run `picocom -b 9600 --omap crlf --imap lfcrlf /dev/ttyACM<number>`

This program, unlike many others, does read the identification rom of the DS2502, so it will give a comprehensive error when it finds a device that is not a DS2502 instead of a CRC one. This also allows it to handle having multiple DS2502s on the same OneWire bus.

The software has two main limitations. It doesn't support writing to the status register or crc checks on subsequent writes. Adding support for the former would be relatively simple, but I can't find an example of where the latter works.

If you get a CRC error where the DS2052 CRC is FF then it likely means that the DS2502 was disconnected. This is because the OneWire bus is pulled up to a logical 1 and if there is no DS2502 to bring it to ground it will read 0xFF (0b11111111).

## Hardware
The programmer schematic in the hardware folder is just a copy of the first circuit of figure 8 in the [DS2502 datasheet](https://www.analog.com/media/en/technical-documentation/data-sheets/DS2502.pdf). The PCB is just a layed out version of the programmer. The arduino schematic shows how to hook up an Arduino to a DS2502 for reading only.

Pin 6 on the Arduino is the OneWire pin and pin 7 is the programming one.

If you are trying to order a DS2502, but can't find any a DS2502+ is functionally equivalent.

## Simulator

For ease of testing there is a simulator file included for [SimulIDE](https://simulide.com/p/). This simulator uses three Arduino Unos. One runs this software and the other two run [OneWireHub](https://github.com/orgua/OneWireHub)'s DS2502 emulator example sketch. This library is and Arduino library and can be found in the IDE library manager. OneWireHub does have some issues though. Writing to it having multiple devices on the same bus both don't work. The former issue is reported [here](https://github.com/orgua/OneWireHub/issues/137). Real hardware is required to test those functions.

The best way I found to get access to the serial console of the Arduino in SimulIDE was to use the Serial Port and socat to loop it back to a serial terminal. To set this up run  
1. `socat -d -d pty,raw,echo=0 pty,raw,echo=0`
2. `picocom -b 9600 --omap crlf --imap lfcrlf /dev/pts/<pts one from socat>`
3. `sudo simulide`
4. Open the circuit
5. Open the serial port properties and put `/dev/pts/<pts two from socat>` in there
6. Press open on the serial port

## Useful Links

There are many useful resources that I used to complete this project. Here are a few of them
- https://github.com/orgua/OneWireHub/blob/main/examples/DS2502_DELLCHG/readme.md
- https://github.com/orgua/OneWireHub/blob/main/examples/DS2502_DELLCHG/DS2502_DELLCHG.ino
- https://github.com/KivApple/dell-charger-emulator
- https://github.com/garyStofer/DS2502_DELL_PS
- https://nickschicht.wordpress.com/2009/07/15/dell-power-supply-fault/
- https://web.archive.org/web/20171231075131/http://www.laptop-junction.com/toast/content/dell-ac-power-adapter-not-recognized

## License 
Everything except the software folder is licensed under Creative Commons Attribution-ShareAlike 4.0 International
The software folder is licensed under the GPLv3