#include <Arduino.h>
#include "PinConfig.h"
#include "IRHandler.h"
#include "Current.h"
#include "Voltage.h"
#include "display.h"
#include "WebClient.h"

// ===================== CONFIGURATION =====================
const char* WIFI_SSID = "RCB";
const char* WIFI_PASSWORD = "http@007";
const char* SERVER_URL = "http://192.168.31.222:5000";

// Sensor Calibration Constants
const float slope_1 = 0.0046521;    
const float intercept_1 = -0.1628;
const float slope_2 = 0.004156;     
const float intercept_2 = -0.1646;
const float slope_3 = 0.002896;     
const float intercept_3 = -0.07227;

// Voltage Sensor Configuration
const uint8_t VOLTAGE_PIN = 35;
const float Vref = 3.3;
const float VOLTAGE_CALIBRATION = 890.0;

// ===================== CREATE INSTANCES =====================
PinConfig pinConfig;
CurrentSensor sensor1(32, slope_1, intercept_1);
CurrentSensor sensor2(33, slope_2, intercept_2);
CurrentSensor sensor3(34, slope_3, intercept_3);
VoltageSensor voltageSensor(VOLTAGE_PIN, Vref, VOLTAGE_CALIBRATION);
IRHandler irHandler(pinConfig);
Display display;
WebClient webClient(WIFI_SSID, WIFI_PASSWORD, SERVER_URL);

// ===================== TIMING VARIABLES =====================
unsigned long printPeriod = 1500;           // Print every 1.5 seconds
unsigned long previousMillis = 0;
unsigned long webSendPeriod = 10000;        // Send sensor data every 10 seconds
unsigned long previousWebMillis = 0;
unsigned long relayPollPeriod = 3000;       // Poll relay state every 3 seconds
unsigned long previousRelayPoll = 0;

// ===================== GLOBAL VARIABLES =====================
float lastVoltage = 0;
float lastCurrent1 = 0;
float lastCurrent2 = 0;
float lastCurrent3 = 0;
float lastTotalCurrent = 0;
float lastPower1 = 0;
float lastPower2 = 0;
float lastTotalPower = 0;

// Track previous relay states to detect IR remote changes
bool previousRelay1State = false;
bool previousRelay2State = false;
bool initialSyncDone = false;

// ===================== SETUP =====================
void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n\n========================================");
    Serial.println("⚡ SMART ENERGY METER");
    Serial.println("   with Synchronized Relay Control");
    Serial.println("========================================\n");
    delay(2000);
    
    // Initialize Current Sensors
    Serial.println("📊 Initializing current sensors...");
    sensor1.begin();
    sensor2.begin();
    sensor3.begin();

    // Calibrate Current Sensors
    Serial.println("🔧 Calibrating current sensors...");
    Serial.println("   Ensure NO LOAD is connected!");
    delay(1000);
    sensor1.calibrate(1000);
    delay(500);
    sensor2.calibrate(1000);
    delay(500);
    sensor3.calibrate(1000);
    Serial.println("✅ Calibration complete\n");
    
    // Initialize Voltage Sensor
    Serial.println("⚡ Initializing voltage sensor...");
    voltageSensor.begin();
    
    // Initialize Hardware
    Serial.println("🔌 Initializing relay control...");
    pinConfig.begin();
    
    Serial.println("📡 Initializing IR receiver...");
    irHandler.begin();
    
    Serial.println("📺 Initializing LCD display...");
    display.begin();
    
    // Initialize WiFi
    webClient.begin();
    
    // Initial relay state sync with server
    if (webClient.isConnected()) {
        Serial.println("🔄 Performing initial relay state sync...");
        previousRelay1State = pinConfig.getRelay1State();
        previousRelay2State = pinConfig.getRelay2State();
        webClient.postRelayState(previousRelay1State, previousRelay2State);
        initialSyncDone = true;
    }
    
    Serial.println("\n========================================");
    Serial.println("✅ SYSTEM READY");
    Serial.println("========================================");
    Serial.println("Control Methods:");
    Serial.println("  • IR Remote: Instant local control + server sync");
    Serial.println("  • Web Dashboard: Server commands");
    Serial.println("\nData Flow:");
    Serial.println("  • Sensors → Server: Every 10s");
    Serial.println("  • IR Change → Server: Immediate POST");
    Serial.println("  • Server → ESP32: Every 3s (poll for web commands)");
    Serial.println("========================================\n");
    
    delay(1000);
}

// ===================== MAIN LOOP =====================
void loop() {
    // Maintain WiFi connection
    webClient.maintain();
    
    // ========== IR REMOTE CONTROL (Priority) ==========
    // Check if IR remote triggered a relay change
    if (irHandler.update()) {
        // IR remote changed a relay - sync immediately with server
        bool currentRelay1 = pinConfig.getRelay1State();
        bool currentRelay2 = pinConfig.getRelay2State();
        
        Serial.println("📡 IR remote triggered - syncing with server...");
        
        if (webClient.isConnected()) {
            webClient.postRelayState(currentRelay1, currentRelay2);
        }
        
        // Update tracking variables
        previousRelay1State = currentRelay1;
        previousRelay2State = currentRelay2;
    }


    // Continuously update current sensors
    sensor1.update();
    sensor2.update();
    sensor3.update();

    // ========== PRINT READINGS AND UPDATE DISPLAY ==========
    if ((unsigned long)(millis() - previousMillis) >= printPeriod) {
        previousMillis = millis();
        
        Serial.println("\n========== READINGS ==========");
        
        // Get readings
        lastCurrent1 = sensor1.getCurrent(1);
        lastCurrent2 = sensor2.getCurrent(2);
        lastCurrent3 = sensor3.getCurrent(3);
        lastVoltage = voltageSensor.getRmsVoltage();
        
        Serial.print("Voltage: ");
        Serial.print(lastVoltage, 2);
        Serial.println(" V");

        lastTotalCurrent = lastCurrent1 + lastCurrent2;
        Serial.print("Total Current: ");
        Serial.print(lastTotalCurrent, 3);
        Serial.println(" A");

        // Calculate power
        lastPower1 = lastVoltage * lastCurrent1;
        lastPower2 = lastVoltage * lastCurrent2;
        lastTotalPower = lastPower1 + lastPower2;
        
        Serial.print("Total Power: ");
        Serial.print(lastTotalPower, 2);
        Serial.println(" W");
        
        // Display relay states
        Serial.print("Relays: R1=");
        Serial.print(pinConfig.getRelay1State() ? "ON" : "OFF");
        Serial.print(" | R2=");
        Serial.println(pinConfig.getRelay2State() ? "ON" : "OFF");
        
        Serial.println("==============================\n");

        // Update LCD
        display.showCurrents(lastCurrent1, lastCurrent2, lastTotalCurrent);
    }

    // ========== SEND SENSOR DATA TO SERVER ==========
    if ((unsigned long)(millis() - previousWebMillis) >= webSendPeriod) {
        previousWebMillis = millis();
        
        if (webClient.isConnected()) {
            Serial.println("📤 Sending sensor data to server...");
            
            // Send ONLY sensor data (no relay states)
            webClient.sendSensorData(
                lastVoltage,
                lastCurrent1,
                lastCurrent2,
                lastCurrent3,
                lastTotalCurrent,
                lastPower1,
                lastPower2,
                lastTotalPower
            );
        }
    }
    
    // ========== POLL SERVER FOR WEB RELAY COMMANDS ==========
    if ((unsigned long)(millis() - previousRelayPoll) >= relayPollPeriod) {
        previousRelayPoll = millis();
        
        if (webClient.isConnected()) {
            // Get current local states
            bool relay1 = pinConfig.getRelay1State();
            bool relay2 = pinConfig.getRelay2State();
            
            // Check server for new commands (from web dashboard)
            if (webClient.getRelayState(relay1, relay2)) {
                // Server state changed - apply to hardware
                pinConfig.setRelay1(relay1);
                pinConfig.setRelay2(relay2);
                
                Serial.println("✅ Web commands applied to relays");
                
                // Update tracking variables
                previousRelay1State = relay1;
                previousRelay2State = relay2;
            }
        }
    }
}