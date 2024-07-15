// Vyknoll expects these functions to be implemented in the firmware
#pragma once

#include <vyknoll.h>
#include <stdbool.h>

const uint16_t COLOR_BLACK       = 0x0000;      /*   0,   0,   0 */
const uint16_t COLOR_NAVY        = 0x000F;      /*   0,   0, 128 */
const uint16_t COLOR_DARKGREEN   = 0x03E0;      /*   0, 128,   0 */
const uint16_t COLOR_DARKCYAN    = 0x03EF;      /*   0, 128, 128 */
const uint16_t COLOR_MAROON      = 0x7800;      /* 128,   0,   0 */
const uint16_t COLOR_PURPLE      = 0x780F;      /* 128,   0, 128 */
const uint16_t COLOR_OLIVE       = 0x7BE0;      /* 128, 128,   0 */
const uint16_t COLOR_LIGHTGREY   = 0xD69A;      /* 211, 211, 211 */
const uint16_t COLOR_DARKGREY    = 0x7BEF;      /* 128, 128, 128 */
const uint16_t COLOR_BLUE        = 0x001F;      /*   0,   0, 255 */
const uint16_t COLOR_GREEN       = 0x07E0;      /*   0, 255,   0 */
const uint16_t COLOR_CYAN        = 0x07FF;      /*   0, 255, 255 */
const uint16_t COLOR_RED         = 0xF800;      /* 255,   0,   0 */
const uint16_t COLOR_MAGENTA     = 0xF81F;      /* 255,   0, 255 */
const uint16_t COLOR_YELLOW      = 0xFFE0;      /* 255, 255,   0 */
const uint16_t COLOR_WHITE       = 0xFFFF;      /* 255, 255, 255 */
const uint16_t COLOR_ORANGE      = 0xFDA0;      /* 255, 180,   0 */
const uint16_t COLOR_GREENYELLOW = 0xB7E0;      /* 180, 255,   0 */
const uint16_t COLOR_PINK        = 0xFE19;      /* 255, 192, 203 */ //Lighter pink, was 0xFC9F
const uint16_t COLOR_BROWN       = 0x9A60;      /* 150,  75,   0 */
const uint16_t COLOR_GOLD        = 0xFEA0;      /* 255, 215,   0 */
const uint16_t COLOR_SILVER      = 0xC618;      /* 192, 192, 192 */
const uint16_t COLOR_SKYBLUE     = 0x867D;      /* 135, 206, 235 */
const uint16_t COLOR_VIOLET      = 0x915C;      /* 180,  46, 226 */

// Setup
void HALSetup(void);
void HALTriggerError(void);
bool HALErrorTriggered(void);
void SystemReboot(void);

// Persisted State
bool PersistedStateGet(vyknoll_persisted_config_t* config);
bool PersistedStateSave(vyknoll_persisted_config_t* config);

// LCD Display
bool LcdTurnOff(void);
bool LcdTurnOn(void);
bool LcdClearDisplay(void);
bool LcdClearDisplay(uint16_t color);
bool LcdDrawText(const char* text);
bool LcdDrawText(const char* text, uint16_t y);
bool LcdDrawText(const char* text, uint16_t x, uint16_t y);

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

// Owntone API
typedef void (*api_callback_t)(uint16_t code, const char* response);
bool GetRequest(const char* url, api_callback_t callback);
bool PutRequest(const char* url, api_callback_t callback);
bool PostRequest(const char* url, const char* body, api_callback_t callback);