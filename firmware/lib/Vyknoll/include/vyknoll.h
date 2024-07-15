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
  QUERYING,
  QUEUING,
  WIFI_SETUP,
  QUEUED,
  PLAY,
  PLAY_EXTERN,
  PLAYING,
  PAUSE,
  PAUSE_EXTERN,
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
  HAL_ERROR,
} vyknoll_states_t;

#define CONFIG_MAGIC 0x56594B4E4F4C4C21

typedef struct {
  uint64_t magic; // "Vyknoll!"
  uint8_t version; // 0 
  char ssid[64]; // WiFi SSID
  char password[64]; // WiFi password
  char server[16]; // Server IP address
  uint16_t port; // Server port
} vyknoll_persisted_config_t;

void StateMachineTick(void);
vyknoll_states_t GetState();
void StateMachineSetup(void);