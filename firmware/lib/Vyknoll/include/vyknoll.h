#include <stdint.h>

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