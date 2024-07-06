#include <Arduino.h>
#include <vyknoll.h>
#include <esp_system.h>
#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <cstring>
#include <list>

void setup() {
    // TODO: setup
}

void loop() {
    // We just tick the state machine
    StateMachineTick();
}