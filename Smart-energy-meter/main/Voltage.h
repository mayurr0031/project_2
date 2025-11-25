#ifndef VOLTAGE_H
#define VOLTAGE_H

#include <Arduino.h>
#include <ZMPT101B.h>

// Voltage Sensor Class wrapper for ZMPT101B library
class VoltageSensor {
private:
    // ==================== PRIVATE VARIABLES ====================
    uint8_t sensorPin;
    float vref;
    float sensitivity;
    ZMPT101B* sensor;
    bool initialized;

public:
    // Constructor
    VoltageSensor(uint8_t pin, float reference = 3.3, float calibration = 890.0) 
        : sensorPin(pin), 
          vref(reference),
          sensitivity(calibration),
          sensor(nullptr),
          initialized(false) {}

    // Destructor
    ~VoltageSensor() {
        if (sensor != nullptr) {
            delete sensor;
        }
    }

    // Initialize sensor
    void begin() {
        if (!initialized) {
            // Create ZMPT101B sensor object
            sensor = new ZMPT101B(sensorPin, vref);
            
            // Set calibration sensitivity
            sensor->setSensitivity(sensitivity);
            
            initialized = true;
            
            Serial.print("Voltage Sensor initialized on pin ");
            Serial.println(sensorPin);
        }
    }

    // Get RMS voltage (directly calls library function)
    float getRmsVoltage() {
        if (!initialized || sensor == nullptr) {
            return 0.0;
        }
        return sensor->getRmsVoltage();
    }

    // Set calibration sensitivity
    void setSensitivity(float factor) {
        sensitivity = factor;
        if (initialized && sensor != nullptr) {
            sensor->setSensitivity(sensitivity);
        }
    }

    // Get current sensitivity value
    float getSensitivity() const {
        return sensitivity;
    }

    // Check if sensor is initialized
    bool isInitialized() const {
        return initialized;
    }

    // Print voltage reading to Serial
    void printVoltage() {
        if (!initialized) return;
        
        float voltage = getRmsVoltage();
        Serial.print("AC RMS Voltage: ");
        Serial.print(voltage);
        Serial.println(" V");
    }
};

#endif // VOLTAGE_H