#include <stdint.h>
#include <stdbool.h>

#pragma once

typedef enum {
  INIT = 0,
  WAITING_FOR_WIFI,
  CHECK_OTA,
  CHECK_POWERSWITCH,
  CHECK_OWNTONE,
  WAITING_FOR_QUEUE,
  SPINNING,
  OTA,
  QUEUING,
  WIFI_SETUP,
  QUEUED,
  PLAYING,
  PAUSED,
  FINISHED,
  DONE,

  OFF,
  DRAINING,

  // Special state for rebooting
  REBOOT,

  // ERROR states
  ERROR = 100,
  TAG_ERROR,
  QUEUE_ERROR,
  OTA_ERROR,
} vyknoll_states_t;

typedef struct {
  char magic[8]; // "Vyknoll"
  uint8_t version; // 0 
  char ssid[64]; // WiFi SSID
  char password[64]; // WiFi password
  char server[16]; // Server IP address
  uint16_t port; // Server port
} vyknoll_persisted_config_t;

void StateMachineTick(void);
vyknoll_states_t GetState();