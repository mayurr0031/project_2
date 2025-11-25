#ifndef PIN_CONFIG_H
#define PIN_CONFIG_H

#include <Arduino.h>

// Relay Pin Configuration and Control Class
class PinConfig {
private:
    // Pin definitions
    static const uint8_t RELAY1_PIN = 26;
    static const uint8_t RELAY2_PIN = 25;
    
    // Internal state tracking
    bool relay1State;
    bool relay2State;
    bool initialized;

    // Helper function for pin writing (handles active-LOW logic)
    void writeRelay(uint8_t pin, bool state) {
        digitalWrite(pin, state ? LOW : HIGH);  // Active-LOW relay logic
    }

public:
    // Constructor
    PinConfig() : relay1State(false), relay2State(false), initialized(false) {}

    // Initialize pins
    void begin() {
        if (!initialized) {
            pinMode(RELAY1_PIN, OUTPUT);
            pinMode(RELAY2_PIN, OUTPUT);
            
            // Initial state: relays OFF (HIGH for active-LOW relays)
            writeRelay(RELAY1_PIN, false);
            writeRelay(RELAY2_PIN, false);
            
            relay1State = false;
            relay2State = false;
            initialized = true;
        }
    }

    // Toggle functions
    void toggleRelay1() {
        if (!initialized) return;
        relay1State = !relay1State;
        writeRelay(RELAY1_PIN, relay1State);
    }

    void toggleRelay2() {
        if (!initialized) return;
        relay2State = !relay2State;
        writeRelay(RELAY2_PIN, relay2State);
    }

    // Direct control functions
    void setRelay1(bool on) {
        if (!initialized) return;
        relay1State = on;
        writeRelay(RELAY1_PIN, relay1State);
    }

    void setRelay2(bool on) {
        if (!initialized) return;
        relay2State = on;
        writeRelay(RELAY2_PIN, relay2State);
    }

    // State getters
    bool getRelay1State() const {
        return relay1State;
    }

    bool getRelay2State() const {
        return relay2State;
    }

    // Check initialization status
    bool isInitialized() const {
        return initialized;
    }
};

#endif // PIN_CONFIG_H
