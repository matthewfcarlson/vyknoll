#include <stdint.h>
#include <stdbool.h>

#pragma once

typedef enum {
  INIT = 0,
  SETUP,
  WAITING_FOR_WIFI,
  WAITING_FOR_QUEUE,
  CHECK,
  SPINNING,

  // ERROR states
  ERROR = 100,
  TAG_ERROR,
  QUEUE_ERROR,
} vyknoll_states_t;

typedef struct {
  bool power_switch_on;
  bool turntable_on;
} vyknoll_state_t;

void ProcessState(vyknoll_state_t* state);
vyknoll_states_t GetState();