// The HAL for ESP-32
#include <vyknoll_hal.h>
#include <FS.h>
#include <SPIFFS.h>
#include <esp_system.h>
#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <cstring>
#include <list>


bool spiffsStarted = false;
const char configFilePath[] = "/config.bin";

void HALSetup() {
    // Start SPIFFS and format on failure
    if (SPIFFS.begin(true) == false) {
        // trigger error?
        printf("ERROR: Failed to mount SPIFFS\n");
        HALTriggerError();
    } else {
        spiffsStarted = true;
    }
}

// Read the config from a file
bool PersistedStateGet(vyknoll_persisted_config_t* config) {
    if (!spiffsStarted) return false;
    bzero(config, sizeof(vyknoll_persisted_config_t));
    if (!SPIFFS.exists(configFilePath)) return false;
    File file = SPIFFS.open(configFilePath);
    if (file.size() < sizeof(vyknoll_persisted_config_t)) {
        printf("File is too small, fix");
    }
    file.readBytes((char*)config, sizeof(vyknoll_persisted_config_t));
    file.close();
    return true;
}
// Write the config to a file
bool PersistedStateSave(vyknoll_persisted_config_t* config) {
    if (!spiffsStarted) return false;
    File file = SPIFFS.open(configFilePath, "w", true);
    file.write((uint8_t*)config, sizeof(vyknoll_persisted_config_t));
    file.close();
    return true;
}

// LCD Display
bool LcdTurnOff(void) {
    return true;
}
bool LcdTurnOn(void) {
    // TODO
    return true;
}

// Power Switch
bool PowerSwitchReadIfOn(void) {
    // TODO
    return false;
}
// Needle Switch
bool NeedleSwitchReadIfOn(void) {
    // TODO
    return false;
}

// turntable
bool TurntableSetSpeed(uint8_t speed) {
    // TODO
    return true;
}

// NFC reader
bool NFCReadTag(char* tag_data, uint16_t tag_data_len) {
    return false;
}

// WiFi
bool WiFiConnect(char* ssid, char* password){
    if (WiFi.begin(ssid, password) == WL_CONNECT_FAILED) {
        return false;
    }
    return true;
}
bool WiFiIsConnected(void) {
    return WiFi.status() == WL_CONNECTED;
}