#include <Arduino.h>
// #include "main.h"

// Declared weak in Arduino.h to allow user redefinitions.
int atexit(void (* /*func*/ )()) { return 0; }

// Weak empty variant initialization function.
// May be redefined by variant files.
void initVariant() __attribute__((weak));
void initVariant() { }

void setupUSB() __attribute__((weak));
void setupUSB() { }

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