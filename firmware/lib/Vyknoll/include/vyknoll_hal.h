// Vyknoll expects these functions to be implemented in the firmware
#include <vyknoll.h>
#include <stdbool.h>

// Persisted State
bool PersistedStateGet(vyknoll_persisted_config_t* config);
bool PersistedStateSave(vyknoll_persisted_config_t* config);

// LCD Display
bool LcdTurnOff(void);
bool LcdTurnOn(void);

// Power Switch
bool PowerSwitchReadIfOn(void);
bool NeedleSwitchReadIfOn(void);

// turntable
bool TurntableSetSpeed(uint8_t speed);