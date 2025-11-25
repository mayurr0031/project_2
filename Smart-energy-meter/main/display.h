// Display.h
#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Simple wrapper for a 16x2 I2C LCD using LiquidCrystal_I2C
class Display {
private:
    LiquidCrystal_I2C lcd;
    bool initialized;

public:
    // Constructor: default address 0x27, 16x2 display
    Display(uint8_t address = 0x27, uint8_t cols = 16, uint8_t rows = 2)
      : lcd(address, cols, rows), initialized(false) {}

    // Initialize the display (call in setup)
    void begin() {
        if (initialized) return;
        Wire.begin();
        lcd.init();
        lcd.backlight();
        lcd.clear();
        initialized = true;
    }

    // Show currents: I1, I2 and total on two lines
    void showCurrents(float i1, float i2, float total) {
        if (!initialized) return;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("I:");
        lcd.print(i1, 3);
        lcd.print(" P:");
        lcd.print(i2, 3);

        lcd.setCursor(0, 1);
        lcd.print("Voltage=");
        lcd.print(total, 3);
        lcd.print(" v");
    }
};

#endif // DISPLAY_H