#include <Arduino.h>
#include <vyknoll.h>
#include <vyknoll_hal.h>

void setup() {
    HALSetup();
}

void loop() {
    // We just tick the state machine
    StateMachineTick();
}