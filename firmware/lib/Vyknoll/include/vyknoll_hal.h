// Vyknoll expects these functions to be implemented in the firmware
#include <vyknoll.h>
#include <stdbool.h>

// Setup
void HALSetup(void);
void HALTriggerError(void);
bool HALErrorTriggered(void);

// Persisted State
bool PersistedStateGet(vyknoll_persisted_config_t* config);
bool PersistedStateSave(vyknoll_persisted_config_t* config);

// LCD Display
bool LcdTurnOff(void);
bool LcdTurnOn(void);
bool LcdClearDisplay(void);

// Power Switch
bool PowerSwitchReadIfOn(void);
bool NeedleSwitchReadIfOn(void);

// turntable
bool TurntableSetSpeed(uint8_t speed);

// NFC
bool NFCReadTag(char* tag_data, uint16_t tag_data_len);

// WiFi
bool WiFiConnect(char* ssid, char* password);
bool WiFiIsConnected(void);