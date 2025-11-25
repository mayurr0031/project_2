#ifndef WEB_CLIENT_H
#define WEB_CLIENT_H

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

class WebClient {
private:
    const char* ssid;
    const char* password;
    String serverUrl;
    bool connected;
    unsigned long lastReconnectAttempt;
    const unsigned long reconnectInterval = 30000;
    HTTPClient http;

public:
    WebClient(const char* wifi_ssid, const char* wifi_password, const char* server_url)
        : ssid(wifi_ssid), 
          password(wifi_password),
          serverUrl(server_url),
          connected(false),
          lastReconnectAttempt(0) {}

    // Initialize WiFi connection
    void begin() {
        Serial.println("\n========================================");
        Serial.println("🌐 WebClient: Connecting to WiFi...");
        Serial.print("   SSID: ");
        Serial.println(ssid);
        Serial.println("========================================");
        
        WiFi.mode(WIFI_STA);
        WiFi.begin(ssid, password);
        
        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 20) {
            delay(500);
            Serial.print(".");
            attempts++;
        }
        Serial.println();
        
        if (WiFi.status() == WL_CONNECTED) {
            connected = true;
            Serial.println("\n✅ WiFi Connected Successfully!");
            Serial.print("   IP Address: ");
            Serial.println(WiFi.localIP());
            Serial.print("   Signal Strength: ");
            Serial.print(WiFi.RSSI());
            Serial.println(" dBm");
            Serial.print("   Server URL: ");
            Serial.println(serverUrl);
        } else {
            connected = false;
            Serial.println("\n❌ WiFi Connection Failed!");
            Serial.println("   Please check:");
            Serial.println("   - WiFi SSID and Password");
            Serial.println("   - Router is powered on");
            Serial.println("   - ESP32 is in range");
        }
        Serial.println("========================================\n");
    }

    // Maintain WiFi connection
    void maintain() {
        if (WiFi.status() != WL_CONNECTED) {
            connected = false;
            
            if (millis() - lastReconnectAttempt >= reconnectInterval) {
                lastReconnectAttempt = millis();
                Serial.println("🔄 WebClient: Reconnecting to WiFi...");
                WiFi.disconnect();
                WiFi.begin(ssid, password);
            }
        } else if (!connected) {
            connected = true;
            Serial.println("\n✅ WebClient: WiFi Reconnected!");
            Serial.print("   IP Address: ");
            Serial.println(WiFi.localIP());
        }
    }

    // Send ONLY sensor data to server (no relay states)
    bool sendSensorData(float voltage, float current1, float current2, float current3,
                        float totalCurrent, float power1, float power2, float totalPower) {
        
        if (!connected) {
            Serial.println("❌ WebClient: Not connected to WiFi");
            return false;
        }

        StaticJsonDocument<512> doc;
        doc["voltage"] = voltage;
        doc["current1"] = current1;
        doc["current2"] = current2;
        doc["current3"] = current3;
        doc["total_current"] = totalCurrent;
        doc["power1"] = power1;
        doc["power2"] = power2;
        doc["total_power"] = totalPower;

        String jsonData;
        serializeJson(doc, jsonData);

        String endpoint = serverUrl + "/api/data";
        http.begin(endpoint);
        http.addHeader("Content-Type", "application/json");
        
        int httpResponseCode = http.POST(jsonData);
        
        if (httpResponseCode > 0) {
            Serial.print("✅ Sensor data sent | Response: ");
            Serial.println(httpResponseCode);
            http.end();
            return true;
        } else {
            Serial.print("❌ Failed to send sensor data | Error: ");
            Serial.println(httpResponseCode);
            http.end();
            return false;
        }
    }

    // POST relay state to server (after IR remote change or initialization)
    bool postRelayState(bool relay1State, bool relay2State) {
        if (!connected) {
            return false;
        }

        StaticJsonDocument<128> doc;
        doc["relay1"] = relay1State;
        doc["relay2"] = relay2State;

        String jsonData;
        serializeJson(doc, jsonData);

        String endpoint = serverUrl + "/api/relay/state";
        http.begin(endpoint);
        http.addHeader("Content-Type", "application/json");
        
        int httpResponseCode = http.POST(jsonData);
        
        if (httpResponseCode == 200) {
            Serial.print("✅ Relay state posted: R1=");
            Serial.print(relay1State ? "ON" : "OFF");
            Serial.print(", R2=");
            Serial.println(relay2State ? "ON" : "OFF");
            http.end();
            return true;
        } else {
            Serial.print("❌ Failed to post relay state | HTTP: ");
            Serial.println(httpResponseCode);
            http.end();
            return false;
        }
    }

    // GET relay state from server (polling for web dashboard commands)
    bool getRelayState(bool &relay1State, bool &relay2State) {
        if (!connected) {
            return false;
        }

        String endpoint = serverUrl + "/api/relay/state";
        http.begin(endpoint);
        http.setTimeout(5000);
        
        int httpResponseCode = http.GET();
        
        if (httpResponseCode == 200) {
            String payload = http.getString();
            
            StaticJsonDocument<256> doc;
            DeserializationError error = deserializeJson(doc, payload);
            
            if (!error) {
                bool newRelay1 = doc["relay1"];
                bool newRelay2 = doc["relay2"];
                
                // Check if states changed
                bool changed = false;
                
                if (newRelay1 != relay1State) {
                    relay1State = newRelay1;
                    Serial.print("🌐 Web command received: Relay 1 → ");
                    Serial.println(relay1State ? "ON" : "OFF");
                    changed = true;
                }
                
                if (newRelay2 != relay2State) {
                    relay2State = newRelay2;
                    Serial.print("🌐 Web command received: Relay 2 → ");
                    Serial.println(relay2State ? "ON" : "OFF");
                    changed = true;
                }
                
                http.end();
                return changed;
            } else {
                Serial.println("❌ Failed to parse relay state JSON");
                http.end();
                return false;
            }
        }
        
        http.end();
        return false;
    }

    // Check connection status
    bool isConnected() {
        return connected && (WiFi.status() == WL_CONNECTED);
    }

    // Get WiFi signal strength
    int getSignalStrength() {
        return WiFi.RSSI();
    }

    // Get IP address
    String getIPAddress() {
        return WiFi.localIP().toString();
    }
};

#endif // WEB_CLIENT_H