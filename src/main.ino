#include <Arduino.h>
#include <OneWire/OneWire.h>

/*Notes: 
Uses lf not crlf
Don't forget your 4.7k pullup to 5v for parasite power
*/
/*
DS250x add-only programmable memory reader w/SKIP ROM.

 The DS250x is a 512/1024bit add-only PROM(you can add data but cannot change the old one) that's used mainly for device identification purposes
 like serial number, mfgr data, unique identifiers, etc. It uses the Maxim 1-wire bus.*/
#define progPin 7
OneWire onewire(6);                    // OneWire bus on digital pin 6
String commandString;
#define DS2502SIZE 128 
#define DS2502STATUSSIZE 8
#define bytesPerRow 8
byte dataBuffer[DS2502SIZE];
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
    address,
    confirm
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
    //This makes the program too big, but it is correct. 
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

//TEST: Whether the newlines look good around the CRC error. Is there space before and after it?
bool checkCRC(const char * location, byte* data, byte length, byte deviceCRC = onewire.read()) {
    byte CRC = OneWire::crc8(data, length);
    // byte deviceCRC = onewire.read();
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
    digitalWrite(progPin, HIGH);
    pinMode(progPin, OUTPUT);
    printMenu();

    // Serial.println(onewire.reset());
    // onewire.skip();
    // Serial.println("Hi3!");
}

bool checkStringIsHex(String s) {
    s.toUpperCase();
    for(uint16_t i = 0; i < s.length(); i++ ) {
        if(!isHexadecimalDigit(s.charAt(i))) {
            writeState = none;
            Serial.println("Non hex character found");
            return false;
        }
    }
    return true;
}

void loop() {
    int commandNumber = Serial.read();
    if(commandNumber != -1) {
        if(commandNumber == '\n') {
            Serial.println(commandString);

            if(writeState == input) {
                commandString.replace(" ", "");
                if (checkStringIsHex(commandString)) {
                    boolean first = true;
                    char sub;
                    dataToWriteCount = 0;
                    for(uint16_t i = 0; i < commandString.length(); i++) {
                        char c = commandString.charAt(i);
                        sub = isDigit(c) ? 0x30 : 0x37;
                        
                        if(first) {
                            dataBuffer[dataToWriteCount] = ((c - sub)<<4);
                        } else {
                            dataBuffer[dataToWriteCount] |= (c - sub);
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
                        Serial.println();
                        Serial.println("Enter address to write at");
                        Serial.print("> ");
                        writeState = address;
                        commandString = "";
                        return;
                    }  
                }

            } else if(writeState == address) {
                if(checkStringIsHex(commandString)) {


                    Serial.println();
                    printBytes(dataBuffer, dataToWriteCount, true);
                    Serial.println("Write this to DS2502? (Y,n)");
                    Serial.print("> ");
                    writeState = address;
                    commandString = "";
                    return;
                }

                //ADD: Choose starting address
                //ADD: Check if we overwriting anything before going for it

            } else if(writeState == confirm) {
                commandString.toLowerCase();
                if(commandString.equals("y")) {
                    Serial.println("Writing Data ...");
                    byte writeCommand[4] = {0x0F, 0x00, 0x00, 0x00};    
                  
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
                                break;
                            }
                        } else {
                            break;
                        }  
                        writeCommand[2]++;
                    }
                    
                    //TEST: Make this go again if there is another chip on the bus
                    writeState = none;
                    commandString = "4";
                    return;
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
                byte deviceCount = 0;

                Serial.println();
                while (true) {
                    byte addr[8];
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
                                onewire.write_bytes(statusCommand, sizeof(statusCommand));        // Read data command, leave ghost power on
                                //FIX: Reading the status of the second device gives a crc error
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

                                byte readCommand[3] = {              // array with the commands to initiate a read, DS250x devices expect 3 bytes to start a read: command,LSB&MSB adresses
                                    0xF0, 0x00, 0x00};       // 0xF0 is the Read Data command, followed by 00h 00h as starting address(the beginning, 0000h)
                                onewire.write_bytes(readCommand, sizeof(readCommand));        // Read data command, leave ghost power on
                               
                                //FIX: Reading the data of the second device gives a crc error
                                if (checkCRC("read command", readCommand, sizeof(readCommand))) {      // Then we compare it to the value the ds250x calculated, if it fails, we print debug messages and abort
                                    Serial.println("Data:");   // For the printout of the data 
                                    for (uint16_t i = 0; i < DS2502SIZE; i++) {    // Now it's time to read the PROM data itself, each page is 32 bytes so we need 32 read commandss
                                        dataBuffer[i] = onewire.read();
                                        // Serial.print(onewire.read());       // printout in ASCII
                                        // Serial.print(" ");           // blank space
                                    }
                                    // Serial.println();  
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

            commandString = "";
            Serial.println();
            printMenu();
        } else {
            commandString += (char)commandNumber;
        }
    }
}
