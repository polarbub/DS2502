#include <Arduino.h>
#ifdef CMAKE
#include <OneWire/OneWire.h>
#else
#include <OneWire.h>
#endif

OneWire onewire(6);                    // OneWire bus on digital pin 6

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

void setup() {
    goto lable;

    lable:
    Serial.begin(9600);

    #define progPin 7
    digitalWrite(progPin, HIGH);
    pinMode(progPin, OUTPUT);
    
    #define DS2502SIZE 128 
    byte dataToWriteCount = DS2502SIZE;
    byte dataBuffer[DS2502SIZE];
    byte writeCommand[4] = {0x0F, 0x00, 0x00, 0x00};    
    
    
    if(!onewire.reset()) {
      return;
      delay(1000);
      Serial.println("No device");
    }
    Serial.println("Writing Test");
    onewire.skip();

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
}

void loop() {

}
