#include "Print.h"
#include <Arduino.h>
#include <OneWire/OneWire.h>

// Declared weak in Arduino.h to allow user redefinitions.
int atexit(void (* /*func*/ )()) { return 0; }

// Weak empty variant initialization function.
// May be redefined by variant files.
void initVariant() __attribute__((weak));
void initVariant() { }

void setupUSB() __attribute__((weak));
void setupUSB() { }

/*Notes: 
Uses lf not crlf
Don't forget your 4.7k pullup to 5v for parasite power
*/
/*
DS250x add-only programmable memory reader w/SKIP ROM.

 The DS250x is a 512/1024bit add-only PROM(you can add data but cannot change the old one) that's used mainly for device identification purposes
 like serial number, mfgr data, unique identifiers, etc. It uses the Maxim 1-wire bus.

 This sketch will use the SKIP ROM function that skips the 1-Wire search phase since we only have one device connected in the bus on digital pin 6.
 If more than one device is connected to the bus, it will fail.
 Sketch will not verify if device connected is from the DS250x family since the skip rom function effectively skips the family-id byte readout.
 thus it is possible to run this sketch with any Maxim OneWire device in which case the command CRC will most likely fail.
 Sketch will only read the first page of memory(32bits) starting from the lower address(0000h), if more than 1 device is present, then use the sketch with search functions.
 Remember to put a 4.7K pullup resistor between pin 6 and +Vcc

 To change the range or ammount of data to read, simply change the data array size, LSB/MSB addresses and for loop iterations

 This example code is in the public domain and is provided AS-IS.

 Built with Arduino 0022 and PJRC OneWire 2.0 library http://www.pjrc.com/teensy/td_libs_OneWire.html

 created by Guillermo Lovato <glovato@gmail.com>
 march/2011

 */

OneWire onewire(6);                    // OneWire bus on digital pin 6

bool present;                // device present var
byte readCommand[3] = {              // array with the commands to initiate a read, DS250x devices expect 3 bytes to start a read: command,LSB&MSB adresses
    0xF0 , 0x00 , 0x00   };       // 0xF0 is the Read Data command, followed by 00h 00h as starting address(the beginning, 0000h)
int serialData = 0;
// int command = 0;
String commandString;
byte addr[8];
byte CRC;
byte deviceCRC;
byte deviceCount = 0;

void printMenu() {
    Serial.println("DS2502 Controller");
    Serial.println("--------------------------------------------------------------------------------");
    Serial.println("1) Read status");
    Serial.println("2) Read rom");
    Serial.println("3) Read data");
    Serial.println("4) Write data");
    Serial.print("> ");
}

void setup() {
    Serial.begin(9600);
    printMenu();
}

enum Command {
    invalid = 0,
    readStatus = 1,
    readRom = 2,
    readData = 3,
    writeData = 4
};
Command command;

void loop() {
    serialData = Serial.read();
    if(serialData != -1) {
        if(serialData == '\n') {
            Serial.println(commandString);
            Serial.println();

            if(commandString.length() > 1 || (command = (Command)commandString.toInt()) < 1 || command > 4) {
                Serial.println("Invalid command");

            } else {
                if(command == readStatus) {
                    Serial.println("Read Status");

                } else if (command == readRom) {    
                    Serial.println("Read Rom");
                    
                } else if (command == readData) {
                    Serial.println("Reading Data ...");

                    onewire.reset();           // OneWire bus reset, always needed to start operation on the bus, returns a 1/TRUE if there's a device present.
                    deviceCount = 0;

                    while (true) {
                        if (!onewire.search(addr)) {
                            if (deviceCount == 0) {
                                Serial.println("No DS2502(s) present");
                            } 
                            onewire.reset_search();
                            break;

                        } else {
                            deviceCount++; 
                            if(deviceCount != 1) Serial.println();
                            Serial.print("Device number ");
                            Serial.println(deviceCount, DEC);

                            if ((CRC = OneWire::crc8(addr, 7)) != addr[7]) {
                                Serial.println("Invalid ROM CRC");
                                Serial.print("Calculated CRC:");
                                Serial.println(CRC,HEX);    // HEX makes it easier to observe and compare
                                Serial.print("DS2502 CRC:");
                                Serial.println(addr[7],HEX);
                                break;

                            } else if (addr[0] != 0x09) {
                                Serial.print("Device ID is 0x");
                                Serial.print(addr[0], HEX);
                                Serial.println(" which is not a DS2502 (ID 0x9)");

                            } else {
                                onewire.write_bytes(readCommand, sizeof(readCommand));        // Read data command, leave ghost power on
                                
                                deviceCRC = onewire.read();             // DS250x generates a CRC for the command we sent, we assign a read slot and store it's value
                                CRC = OneWire::crc8(readCommand, sizeof(readCommand));  // We calculate the CRC of the commands we sent using the library function and store it

                                if (CRC != deviceCRC) {      // Then we compare it to the value the ds250x calculated, if it fails, we print debug messages and abort
                                    Serial.println("Invalid command CRC");
                                    Serial.print("Calculated CRC:");
                                    Serial.println(CRC,HEX);    // HEX makes it easier to observe and compare
                                    Serial.print("DS250x readback CRC:");
                                    Serial.println(deviceCRC,HEX);
                                    // Since CRC failed, we abort the rest of the loop and start over
                                } else {
                                    //ADD: Make this print out like a hex monitor
                                    Serial.println("Data is: ");   // For the printout of the data 
                                    for (byte i = 0; i < 32; i++) {    // Now it's time to read the PROM data itself, each page is 32 bytes so we need 32 read commandss
                                        Serial.print(onewire.read());       // printout in ASCII
                                        Serial.print(" ");           // blank space
                                    }
                                    Serial.println();   
                                }
                            }
                        } 
                    }
                } else if (command == writeData) {
                    Serial.println("Write Data");
                }         
            }
            commandString = "";
            Serial.println();
            printMenu();
        } else {
            commandString += (char)serialData;
        }
    }
}

int main() {
    init();

    initVariant();

#if defined(USBCON)
    USBDevice.attach();
#endif

    setup();
    while (true) {
        loop();

        //DON'T TOUCH
        if (serialEventRun) serialEventRun();
    }

    return 0;
}
