#ifndef IR_HANDLER_H
#define IR_HANDLER_H

#include <Arduino.h>
#include <IRremote.h>
#include "PinConfig.h"

// IR Remote Control Handler Class
class IRHandler {
private:
    // ==================== CONSTANTS ====================
    static const uint8_t IR_RECEIVE_PIN = 19;
    static const unsigned long IR_CODE_RELAY1 = 0xA758FF00;
    static const unsigned long IR_CODE_RELAY2 = 0xBB44FF00;
    
    // ==================== PRIVATE VARIABLES ====================
    unsigned long lastCode;
    bool initialized;
    PinConfig& pinConfig;  // Reference to PinConfig instance

public:
    // Constructor taking PinConfig reference
    IRHandler(PinConfig& config) : lastCode(0), initialized(false), pinConfig(config) {}

    // Initialize IR receiver
    void begin() {
        if (!initialized) {
            IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK);
            initialized = true;
            Serial.println("IR Receiver ready...");
        }
    }

    // Process IR input - returns true if a valid command was processed
    bool update() {
        if (!initialized || !pinConfig.isInitialized()) return false;
        if (!IrReceiver.decode()) return false;

        unsigned long code = IrReceiver.decodedIRData.decodedRawData;

        // NEC repeat code handling
        if (code == 0xFFFFFFFF) {
            code = lastCode; // repeat the last valid code
        }

        bool processed = false;
        if (code != 0) {
            if (code == IR_CODE_RELAY1) {
                pinConfig.toggleRelay1();
                Serial.println(pinConfig.getRelay1State() ? "Relay1 ON" : "Relay1 OFF");
                processed = true;
            }
            else if (code == IR_CODE_RELAY2) {
                pinConfig.toggleRelay2();
                Serial.println(pinConfig.getRelay2State() ? "Relay2 ON" : "Relay2 OFF");
                processed = true;
            }

            if (processed) {
                lastCode = code;
                
            }
        }

        IrReceiver.resume();
        return processed;
    }

    // Get the last received code
    unsigned long getLastCode() const {
        return lastCode;
    }

    // Check if IR receiver is initialized
    bool isInitialized() const {
        return initialized;
    }
};

#endif // IR_HANDLER_H
