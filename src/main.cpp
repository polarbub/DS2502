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
//FIX: Put these into the functions that they are used in
bool present;                // device present var
byte readCommand[3] = {              // array with the commands to initiate a read, DS250x devices expect 3 bytes to start a read: command,LSB&MSB adresses
    0xF0, 0x00, 0x00};       // 0xF0 is the Read Data command, followed by 00h 00h as starting address(the beginning, 0000h)
int serialData = 0;
// int command = 0;
String commandString;
byte addr[8];
byte CRC;
// byte deviceCRC;
byte deviceCount = 0;
uint16_t i = 0;

#define DS2502SIZE 128
byte dataToWrite[DS2502SIZE];
uint16_t dataToWriteCount;

enum Command {
    invalid = 0,
    readStatus = 1,
    readRom = 2,
    readData = 3,
    writeData = 4
};
Command command;

enum {
    none,
    input,
    confirm
} writeState;

void printBytes(const uint8_t* toPrint, uint8_t count, bool newline=false) {
  for (uint8_t i = 0; i < count; i++) {
    Serial.print(toPrint[i]>>4, HEX);
    Serial.print(toPrint[i]&0b00001111, HEX);
  }
  if (newline)
    Serial.println();
}

void printMenu() {
    Serial.println("DS2502 Controller");
    Serial.println("--------------------------------------------------------------------------------");
    Serial.println("1) Read status");
    Serial.println("2) Read rom");
    Serial.println("3) Read data");
    Serial.println("4) Write data");
    Serial.print("> ");
}

//TEST: Does the crc check work?
bool checkCRC(const char * location, byte* data, byte length = sizeof(data)) {
    byte CRC = OneWire::crc8(data, length);
    byte deviceCRC = onewire.read();
    if (CRC != deviceCRC) {
        Serial.print("Invalid ");
        Serial.print(location);
        Serial.println(" CRC");
        Serial.print("Calculated CRC: ");
        Serial.println(CRC,HEX);    // HEX makes it easier to observe and compare
        Serial.print("DS2502 CRC: ");
        Serial.println(deviceCRC,HEX);
        return false;
    }
    return true;
}

void setup() {
    Serial.begin(9600);
    printMenu();
}

void loop() {
    serialData = Serial.read();
    if(serialData != -1) {
        if(serialData == '\n') {
            Serial.println(commandString);

            if(writeState == input) {
                boolean safe = true;
                // for(char c : commandString) {
                commandString.replace(" ", "");
                commandString.toUpperCase();
                for(i = 0; i < commandString.length(); i++ ) {
                    if(!isHexadecimalDigit(commandString.charAt(i))) {
                        writeState = none;
                        Serial.println("Non hex character found");
                        safe = false;
                        break;
                    }
                }

                boolean first = true;
                char sub;
                if (safe) {
                    dataToWriteCount = 0;
                    for(i = 0; i < commandString.length(); i++) {
                        char c = commandString.charAt(i);
                        sub = isDigit(c) ? 0x30 : 0x37;
                        
                        if(first) {
                            dataToWrite[dataToWriteCount] = ((c - sub)<<4);
                        } else {
                            dataToWrite[dataToWriteCount] |= (c - sub);
                            dataToWriteCount++;
                        }     
                        
                        first = !first;
                    }
                    if(!first) {
                        Serial.println("Odd number of nibbles (hex characters). Check you didn't remove any leading zeros");
                        writeState = none;
                    } else if(dataToWriteCount == 0) {
                        Serial.print("> ");
                        commandString = "";
                        return;
                    } else {
                         //ADD: Nice looking hex editor output for write confirm
                        Serial.println();
                        printBytes(dataToWrite, dataToWriteCount, true);
                        Serial.println("Write this to DS2502? (Y,n)");
                        Serial.print("> ");
                        writeState = confirm;
                        commandString = "";
                        return;
                    }  
                }

            } else if(writeState == confirm) {
                commandString.toLowerCase();
                if(commandString.equals("y")) {
                    //ADD: Check if we overwriting anything before going for it
                    //ADD: Choose starting address
                    
                    Serial.println("Writing Data ...");
                    byte writeCommand[4] = {0x0F, 0x00, 0x00, dataToWrite[0]};    
                    onewire.write_bytes(writeCommand, sizeof(writeCommand));        // Read data command, leave ghost power on
                    
                    if (checkCRC("command", writeCommand)) {
                        byte deviceReadBack = onewire.read();
                        if(deviceReadBack == dataToWrite[0]) {
                            for(byte i = 1; i < dataToWriteCount; i++) {
                                byte toWrite = dataToWrite[i];
                                onewire.write(toWrite);
                                writeCommand[3] = toWrite;
                                writeCommand[2]++;
                                checkCRC("command", writeCommand);
                                deviceReadBack = onewire.read();
                                if(toWrite != deviceReadBack) {
                                    Serial.print("Incorect readback from DS2502 in position ");
                                    Serial.println(i);
                                    Serial.print("Written Data: ");
                                    Serial.println(dataToWrite[0],HEX);    // HEX makes it easier to observe and compare
                                    Serial.print("Read data: ");
                                    Serial.println(deviceReadBack,HEX); 
                                }
                            }
                        } else {
                            Serial.println("Incorect readback from DS2502 in position 0");
                            Serial.print("Written Data: ");
                            Serial.println(dataToWrite[0],HEX);    // HEX makes it easier to observe and compare
                            Serial.print("Read data: ");
                            Serial.println(deviceReadBack,HEX);
                        }
                        
                         //ADD: Make this print out like the Usagi electric sector checker. Good is . bad is x
                        
                    }
                    Serial.println();

                    //FIX: Make this go again if there is another chip on the bus
                    writeState = none;
                } else if (commandString.equals("n")) {
                    writeState = none;
                    commandString = "";
                    printMenu();
                    return;
                } else {
                    Serial.print("> ");
                    commandString = "";
                    return;
                }
                
            } else if(commandString.length() > 1 || (command = (Command)commandString.toInt()) < 1 || command > 4) {
                Serial.println("Invalid command");

            } else {
                onewire.reset();           // OneWire bus reset, always needed to start operation on the bus, returns a 1/TRUE if there's a device present.
                onewire.reset_search();
                deviceCount = 0;

                Serial.println();
                while (true) {
                    if (!onewire.search(addr)) {
                        if (deviceCount == 0) {
                            Serial.println("No DS2502(s) present");
                        } 
                        break;

                    } else {
                        deviceCount++; 
                        if(deviceCount != 1) Serial.println();
                        Serial.print("Device number ");
                        Serial.println(deviceCount, DEC);

                        if (!checkCRC("command", addr, 7)) {                            
                            break;

                        } else if (addr[0] != 0x09) {
                            Serial.print("Device ID is 0x");
                            Serial.print(addr[0], HEX);
                            Serial.println(" which is not a DS2502 (ID 0x9)");

                        } else {
                            if(command == readStatus) {
                                Serial.println("Reading Status ...");
                                Serial.println("Not implemented yet");

                            } else if (command == readRom) {    
                                Serial.println("Reading Rom ...");
                                Serial.print("ID: 0x");
                                printBytes(addr + 1, 6, true);

                            } else if (command == readData) {
                                Serial.println("Reading Data ...");                                
                                onewire.write_bytes(readCommand, sizeof(readCommand));        // Read data command, leave ghost power on
                               
                                if (checkCRC("command", readCommand)) {      // Then we compare it to the value the ds250x calculated, if it fails, we print debug messages and abort
                                     //ADD: Make this print out like a hex monitor
                                    Serial.println("Data is: ");   // For the printout of the data 
                                    //TEST: Test that the read reads the whole flash and is not like one byte short
                                    for (i = 0; i < DS2502SIZE; i++) {    // Now it's time to read the PROM data itself, each page is 32 bytes so we need 32 read commandss
                                        Serial.print(onewire.read());       // printout in ASCII
                                        Serial.print(" ");           // blank space
                                    }
                                    Serial.println();  
                                } 

                            } else if (command == writeData) {
                                Serial.println("Enter data to write in Hex. Make sure to have preceding zeros and to remove 0x from the start or h from the end. Spaces are optional");
                                Serial.print("> ");
                                commandString = "";
                                writeState = input;
                                return;
                            }        
                        }
                    } 
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
