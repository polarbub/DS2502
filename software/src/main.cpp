/* 
DS2502 is a multitool for DS2502 reading and programming
Copyright (C) 2024 polarbub

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License version 3 as published by
the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <Arduino.h>
#ifdef CMAKE
#include <OneWire/OneWire.h>
#else
#include <OneWire.h>
#endif

#define progPin 7
OneWire onewire(6);                    // OneWire bus on digital pin 6
String commandString;
#define DS2502SIZE 128 
#define DS2502STATUSSIZE 8
#define bytesPerRow 8
byte dataBuffer[DS2502SIZE];
byte extraDataBuffer[DS2502SIZE];
uint16_t dataToWriteCount;
uint8_t Address;
byte deviceCount;
// bool writeAgain = false;

//ADD: Status write command
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
    address,
    confirm,
    restart
} writeState;

void oldPrintBytes(const uint8_t* toPrint, uint8_t count, bool newline=false) {
  for (uint8_t i = 0; i < count; i++) {
    Serial.print(toPrint[i]>>4, HEX);
    Serial.print(toPrint[i]&0b00001111, HEX);
  }
  if (newline)
    Serial.println();
}

uint8_t getHexAddressLength(uint8_t address) {
    //This makes the program too big as it has to link in the software float code, but it is correct. 
    // return ceilf(logf(address) / logf(2) / 4);
    if(address >= 0x10) {
        return 2;
    } else {
        return 1;
    }
}

void printBytes(const uint8_t* toPrint, uint8_t count, bool newline=false) {
    uint8_t maxAddressLength = getHexAddressLength(count);

    for (uint8_t i = 0; i < count; i++) {
        if(i % bytesPerRow == 0) {
            if(i != 0) {
                Serial.print("| ");
                for(uint8_t ii = bytesPerRow; 0 < ii; ii--) {
                    if(isprint(toPrint[i - ii])) {
                        Serial.print((char) toPrint[i - ii]);
                    } else {
                        Serial.print(".");
                    }
                }
                Serial.println();
            }
            
            uint8_t addressLength = getHexAddressLength(i);
            for(int ii = maxAddressLength - addressLength; ii > 0; ii--) {
                Serial.print(' ');
            }
            Serial.print(i, HEX);
            Serial.print(" | ");
        }

        Serial.print(toPrint[i]>>4, HEX);
        Serial.print(toPrint[i]&0b00001111, HEX);
        Serial.print(" ");
    }

    if(count % bytesPerRow != 0) {    
        for(uint8_t i = bytesPerRow - (count % bytesPerRow); 0 < i; i--) {
            Serial.print("   ");
        }
    }

    uint8_t leftover = count % bytesPerRow;
    if(leftover == 0) {
        leftover = 8;
    }
    Serial.print("| ");
    for(; 0 < leftover; leftover--) {
        if(isprint(toPrint[count - leftover])) {
            Serial.print((char) toPrint[count - leftover]);
        } else {
            Serial.print(".");
        }
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

bool checkCRC(const char * location, byte* data, byte length, byte deviceCRC = onewire.read()) {
    byte CRC = OneWire::crc8(data, length);
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

bool stringToHex(String s, byte *buffer, byte maxLength, uint16_t &finalLength) {
    s.toUpperCase();
    for(uint16_t i = 0; i < s.length(); i++ ) {
        if(!isHexadecimalDigit(s.charAt(i))) {
            Serial.println("Non hex character found");
            return false;
        }
    }

    bool first = true;
    char sub;
    finalLength = 0;
    for(uint16_t i = 0; i < s.length(); i++) {
        char c = s.charAt(i);
        sub = isDigit(c) ? 0x30 : 0x37;

        if(first) {
            buffer[finalLength] = ((c - sub)<<4);
        } else {
            buffer[finalLength] |= (c - sub);
            finalLength++;
        }     
        
        first = !first;
    }
    if(!first) {
        Serial.println("Odd number of nibbles (hex characters). Check you didn't remove any leading zeros");    
    }
    return first;
}

bool stringToHex(String s, byte *buffer, byte maxLength) {
    uint16_t finalLength;
    return stringToHex(s, buffer, maxLength, finalLength);
}

void reset(void) {
    commandString = "";
    printMenu();
}

void resetNewline(void) {
    Serial.println();
    reset();
}

bool read(byte* buffer, byte size) {
    byte readCommand[3] = {              // array with the commands to initiate a read, DS250x devices expect 3 bytes to start a read: command,LSB&MSB adresses
        0xF0, 0x00, 0x00};       // 0xF0 is the Read Data command, followed by 00h 00h as starting address(the beginning, 0000h)
    onewire.write_bytes(readCommand, sizeof(readCommand));        // Read data command, leave ghost power on
    if (checkCRC("read command", readCommand, sizeof(readCommand))) {      // Then we compare it to the value the ds250x calculated, if it fails, we print debug messages and abort
        for (uint16_t i = 0; i < size; i++) {    // Now it's time to read the PROM data itself, each page is 32 bytes so we need 32 read commandss
            buffer[i] = onewire.read();
        }
        return true;
    } 
    return false;
}

void setup() {
    Serial.begin(9600);
    digitalWrite(progPin, HIGH);
    pinMode(progPin, OUTPUT);
    printMenu();
}

void loop() {
    int commandNumber = Serial.read();
    if(commandNumber != -1 || writeState == restart) {
        if(commandNumber == '\n' || writeState == restart) {
            if(writeState != restart) Serial.println(commandString);

            if(writeState == input) {
                commandString.replace(" ", "");
                if (stringToHex(commandString, dataBuffer, DS2502SIZE, dataToWriteCount)) {
                    if(dataToWriteCount >= DS2502SIZE) {
                        Serial.println("Too many bytes. DS2502 only has space for 128");
                        writeState = none;
                    } else if(dataToWriteCount == 0) {
                        Serial.print("> ");
                        commandString = "";
                        return;
                    } else {
                        Serial.println();
                        Serial.println("Enter address to write at in hex. Make sure to include all leading zeros");
                        Serial.print("> ");
                        writeState = address;
                        commandString = "";
                        return;
                    }  
                } else {
                    writeState = none;
                }

            } else if(writeState == address) {
                Address = 0;
                if(stringToHex(commandString, &Address, 1)) {
                    uint16_t dataend;
                    if (Address >= DS2502SIZE) {
                        Serial.print("Address 0x");
                        Serial.print(Address, HEX);
                        Serial.println(" is larger than the size of DS2502 (128 Bytes)");
                        writeState = none;
                    } else if((dataend = Address + dataToWriteCount) > DS2502SIZE) {
                        Serial.print("Data end 0x");
                        Serial.print(dataend, HEX);
                        Serial.println(" is larger than the size of DS2502 (128 Bytes)");
                        writeState = none;
                    } else if(read(extraDataBuffer, DS2502SIZE)) {
                        //ADD: Make this do a printbytes thing except it's colored. Red for won't work, yellow for overwritten, green for good
                        Serial.println();
                        Serial.println("Current Data on DS2502");
                        printBytes(extraDataBuffer, DS2502SIZE, true);

                        Serial.println();
                        for(byte i = 0; i < dataToWriteCount; i++) {
                            if(0xFF != extraDataBuffer[i+Address]) {
                                Serial.print("At address 0x");
                                Serial.print(i+Address);
                                Serial.print(" there is already data on the DS2502");
                                byte data = extraDataBuffer[i+Address]&dataBuffer[i];
                                if(data == dataBuffer[i]) {
                                    Serial.println(", but when overwriten the correct data will be there");
                                } else {
                                    Serial.print(" and when overwriten the data will be 0x");
                                    Serial.println(data, HEX);
                                }
                            } 
                        }
                        // stringToHex(commandString, &Address, 1);
                        Serial.println();
                        printBytes(dataBuffer, dataToWriteCount, true);
                        Serial.print("Write this to DS2502 at address 0x");
                        Serial.print(Address, HEX);
                        Serial.println("? (Y,n)");
                        Serial.print("> ");
                        writeState = confirm;
                        commandString = "";
                        return; 
                    } else {
                        writeState = none;
                    }
                } else {
                    writeState = none;
                }
            } else if(writeState == confirm) {
                commandString.toLowerCase();
                if(commandString.equals("y")) {
                    Serial.println("Writing Data ...");
                    byte writeCommand[4] = {0x0F, 0x00, 0x00, 0x00};   
                    bool error = false;
                    writeCommand[2] = Address; 
                  
                    for(byte i = 0; i < dataToWriteCount; i++) {
                        byte toWrite = dataBuffer[i];
                        writeCommand[3] = toWrite;
                     
                        if (i == 0) {
                            onewire.write_bytes(writeCommand, sizeof(writeCommand));        // Read data command, leave ghost power on
                        } else {
                            onewire.write(toWrite);
                        }
                        
                        if(checkCRC("write command", writeCommand, sizeof(writeCommand))) {
                            digitalWrite(progPin, LOW);
                            delayMicroseconds(480);
                            digitalWrite(progPin, HIGH);
                            
                            byte deviceReadBack = onewire.read();
                            if(toWrite != deviceReadBack) {
                                Serial.print("Incorect readback from DS2502 for byte ");
                                Serial.println(i);
                                Serial.print("Written Data: ");
                                Serial.println(dataBuffer[0],HEX);    // HEX makes it easier to observe and compare
                                Serial.print("Read data: ");
                                Serial.println(deviceReadBack,HEX); 
                                writeState = none;
                                resetNewline();
                                return;
                            }
                        } else {
                            writeState = none;
                            resetNewline();
                            return;
                        }  
                        writeCommand[2]++;
                    }

                    writeState = restart;
                    commandString = "4";
                    return;
                } else if (commandString.equals("n")) {
                    writeState = none;
                    // resetNewline();
                    // return;
                } else {
                    Serial.print("> ");
                    commandString = "";
                    return;
                }
                
            } else if(commandString.length() > 1 || (command = (Command)commandString.toInt()) < 1 || command > 4) {
                Serial.println("Invalid command");

            } else {
                if(writeState != restart) {
                    onewire.reset_search();
                    deviceCount = 0;
                }
                writeState = none;
                
                Serial.println();
                while (true) {
                    byte addr[8];
                    //Search resets the bus for us
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

                        // printBytes(addr, 8);
                        if (!checkCRC("rom", addr, 7, addr[7])) {                            
                            break;

                        } else if (addr[0] != 0x09) {
                            Serial.print("Device ID is 0x");
                            Serial.print(addr[0], HEX);
                            Serial.println(" which is not a DS2502 (ID 0x9)");

                        } else {
                            if(command == readStatus) { 
                                Serial.println("Reading Status ...");
                                byte statusCommand[3] = {0xAA, 0x00, 0x00};
                                byte statusData[8];
                                onewire.write_bytes(statusCommand, sizeof(statusCommand));        // Read data command
                                if (checkCRC("status command", statusCommand, sizeof(statusCommand))) {      // Then we compare it to the value the ds250x calculated, if it fails, we print debug messages and abort
                                    for (uint16_t i = 0; i < DS2502STATUSSIZE; i++) {
                                        statusData[i] = onewire.read();
                                    }
                                    if(checkCRC("status data", statusData, sizeof(statusData))) {
                                        Serial.println("Raw status data");
                                        printBytes(statusData, sizeof(statusData), true);
                                        Serial.println();

                                        Serial.println("Page write protection. \"X\" is protected. \".\" is not.\n0123");
                                        Serial.print((statusData[0] & 1) ? "." : "X");
                                        Serial.print((statusData[0]>>1 & 1) ? "." : "X");
                                        Serial.print((statusData[0]>>2 & 1) ? "." : "X");
                                        Serial.print((statusData[0]>>3 & 1) ? "." : "X");
                                        Serial.println('\n');

                                        // Serial.print()
                                        //Print out the page redirection

                                        for(uint16_t i = 1; i <= 4; i++) {
                                            byte b = statusData[i];
                                            if(b == 0xFF) {
                                                Serial.print("No page redirection for page ");
                                                Serial.println(i);
                                            } else {
                                                Serial.print("Page ");
                                                Serial.print(i);
                                                Serial.print(" redirected to 0x");
                                                Serial.println(b);
                                            }
                                        }
                                    }
                                }

                            } else if (command == readRom) {    
                                Serial.println("Reading Rom ...");
                                Serial.print("ID: 0x");
                                printBytes(addr + 1, 6, true);

                            } else if (command == readData) {
                                Serial.println("Reading Data ...");    

                                if(read(dataBuffer, DS2502SIZE)) {
                                    Serial.println("Data:");   // For the printout of the data 
                                    printBytes(dataBuffer, DS2502SIZE, true);
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
            resetNewline();
        } else {
            commandString += (char)commandNumber;
        }
    }
}