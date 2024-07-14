#include <vyknoll.h>
#include <vyknoll_hal.h>
#include <periodic_task.hpp>
#include <synced_property.hpp>
#include <timeout.hpp>
#include <string.h>
#include <iostream>
#include <stdint.h>
#include <stdbool.h>
#include <magic_enum.hpp>

#define MAX_OUTPUTS 4

vyknoll_states_t currentState = INIT;
vyknoll_persisted_config_t config = {0};

// Internal state
char tagData[64] = {0};
SyncedProperty<uint32_t> ownToneVolume = SyncedProperty<uint32_t>(0);
SyncedProperty<uint32_t> ownToneQueueLength = SyncedProperty<uint32_t>(100); // start with a queue size of 100 just because
PeriodicTask* checkOwnToneQueueLength;
PeriodicTask* checkOwnToneVolume;
TimeOut WifiTimeout = TimeOut(30000);
TimeOut QueueTimeout = TimeOut(30000);
char* outputNames[MAX_OUTPUTS] = {0};
bool outputsEnabled[MAX_OUTPUTS] = {0};
static bool _halErrorTriggered = false;


void InternalReset() {
    bzero(tagData, sizeof(tagData));
    bzero(outputsEnabled, sizeof(outputsEnabled));
    for (int i = 0; i < MAX_OUTPUTS; i++) {
        if (outputNames[i] == nullptr) continue;
        free(outputNames[i]);
        outputNames[i] = nullptr;
    }
}

// Hal function
void HALTriggerError(void) {
    _halErrorTriggered = true;
}
bool HALErrorTriggered(void) {
    return _halErrorTriggered;
}

// Tick functions
vyknoll_states_t StateTickInit() {
    if (!PersistedStateGet(&config)){
        return WIFI_SETUP;
    }
    if (config.magic != CONFIG_MAGIC || strnlen(config.ssid, 64) == 0 || strnlen(config.password, 64) == 0 || strnlen(config.server, 16) == 0 || config.port == 0) {
        return WIFI_SETUP;
    }
    return WAITING_FOR_WIFI;
}

vyknoll_states_t StateTickWaitingForWifi() {
    if (WiFiIsConnected()) {
        return CHECK_OTA;
    }
    if (WifiTimeout.Check()) {
        return WIFI_SETUP;
    }
    return WAITING_FOR_WIFI;
}

vyknoll_states_t StateTickCheckOTA() {
    // TODO
    return CHECK_POWERSWITCH;
    // return OTA;
}

vyknoll_states_t StateTickCheckOwntone() {
    // Wait for Owntone to reply with its queue information
    if (checkOwnToneQueueLength != nullptr) checkOwnToneQueueLength->Check();
    if (ownToneQueueLength.GetValue().value == 0) {
        return SPINNING;
    }
    return WAITING_FOR_QUEUE;
}

vyknoll_states_t StateTickCheckPowerSwitch() {
    if (PowerSwitchReadIfOn()) {
        return CHECK_OWNTONE;
    }
    return OFF;
}

vyknoll_states_t StateTickWifiSetup() {
    // Display the wifi setup screen
    // TODO handle
    return WIFI_SETUP;
}

vyknoll_states_t StateTickWaitingForQueue() {
    // Wait for the queue to be empty
    if (checkOwnToneQueueLength != nullptr) checkOwnToneQueueLength->Check();
    if (ownToneQueueLength.GetValue().value == 0) {
        return SPINNING;
    }
    if (QueueTimeout.Check()) {
        return QUEUE_ERROR;
    }
    return WAITING_FOR_QUEUE;
}

vyknoll_states_t StateTickSpinning() {
    // Wait for a tag to be read
    if (NFCReadTag(tagData, sizeof(tagData))) {
        // We got a tag, move on
        // TODO: do we need to issue a lookup first?
        // How are NFC tags encoded?
        return QUEUING;
    }
    return SPINNING;
}

vyknoll_states_t StateTickQueueing() {
    // Wait for the Owntone queue to have some items in it
    return QUEUING;
    // TODO
    return QUEUED;
}
vyknoll_states_t StateTickQueued() {
    // Check if the power switch is off
    if (!PowerSwitchReadIfOn()) {
        return DRAINING;
    }
    // Check if the needle is on
    if (NeedleSwitchReadIfOn()) {
        return PLAYING;
    }
    return QUEUED;
}
vyknoll_states_t StateTickPlaying() {
    // Check if the power switch is off
    if (!PowerSwitchReadIfOn()) {
        return DRAINING;
    }
    // Check if the needle is off
    if (!NeedleSwitchReadIfOn()) {
        return PAUSED;
    }
    return PLAYING;
}

vyknoll_states_t StateTickPaused() {
    // Check if the power switch is off
    if (!PowerSwitchReadIfOn()) {
        return DRAINING;
    }
    // Check if the needle is on
    if (NeedleSwitchReadIfOn()) {
        return PLAYING;
    }
    return PAUSED;
}

vyknoll_states_t StateTickFinished() {
    // Check if the power switch is off
    if (!PowerSwitchReadIfOn()) {
        return OFF;
    }
    // Check if the needle is off
    if (!NeedleSwitchReadIfOn()) {
        return DONE;
    }
    return FINISHED;
}

vyknoll_states_t StateTickDone() {
    // Check if the power switch is off
    if (!PowerSwitchReadIfOn()) {
        return OFF;
    }
    // Check if the needle is on, restart is all
    if (NeedleSwitchReadIfOn()) {
        return SPINNING;
    }
    return DONE;

}

vyknoll_states_t StateTickDraining() {
    // Check if the owntone queue is empty
    // TODO
    return DRAINING;
    return OFF;
}

vyknoll_states_t StateTickReboot() {
    // Reboot the device
    // TODO
    return REBOOT;
}

vyknoll_states_t StateTickError() {
    // Check for a button press, if so, reboot
    if (!PowerSwitchReadIfOn()) {
        return REBOOT;
    }
    return ERROR;
}

vyknoll_states_t StateTickOTA() {
    return OTA;
    // TODO
    // Once OTA has finished, reboot
    return REBOOT;
}

vyknoll_states_t StateTickOff() {
    // Check if the power switch is on
    // Go into a low power mode for a second
    if (PowerSwitchReadIfOn()) return CHECK_POWERSWITCH;
    return OFF;
}

// Transition functions
void StateStartInit() {
    LcdTurnOff();
    return;
}
void StateStartWaitingForWifi() {
    // Connect to wifi with the creds we have stored
    WiFiConnect(config.ssid, config.password);
    // Start the timeout timer
    WifiTimeout.Start();
    return;
}
void StateStartCheckOTA() {
    // Check github for OTA updates
    // TODO: implement this
    return;
}
void StateStartCheckOwnTone() {
    // Check if the Owntone server is up
}
void StateStartWaitingForQueue() {
    // Display the waiting for queue screen
    QueueTimeout.Start();
    LcdTurnOn();
    LcdClearDisplay();
    LcdDrawText("Waiting for Empty Queue");
    return;
}
void StateStartSpinning() {
    // Display the spinning screen
    // TODO
    return;
}
void StateStartOta() {
    // Display the OTA Graphic
    // TODO
}
void StateStartQueuing() {
    // Display the queuing screen
    // TODO
    return;
}
void StateStartPlaying() {
    // Display the playing screen
    // TODO
    return;
}
void StateStartPaused() {
    // Display the paused screen
    // TODO
    return;
}
void StateStartFinished() {
    // Display the finished screen
    // TODO
    return;
}
void StateStartDone() {
    // Display the done screen
    // TODO
    return;
}
void StateStartDraining() {
    // Display the draining screen
    // TODO
    return;
}
void StateStartOff() {
    // Display the off screen
    LcdTurnOff();
    return;
}
void StateStartTagError() {
    // Display the tag error screen
    // TODO
    return;
}
void StateStartOTAError() {
    // Display the ota error screen
    // TODO
    return;
}
void StateStartHalError() {
    // Display the hal error screen
    // TODO
    return;
}
void StateStartQueueError() {
    // Display the queue error screen
    // TODO
    return;
}


void StateMachineTick(void) {
  vyknoll_states_t nextState = currentState;
  switch (currentState) {
        case INIT: nextState = StateTickInit(); break;
        case WAITING_FOR_WIFI: nextState = StateTickWaitingForWifi(); break;
        case CHECK_OTA: nextState = StateTickCheckOTA(); break;
        case OTA: nextState = StateTickOTA(); break;
        case CHECK_POWERSWITCH: nextState = StateTickCheckPowerSwitch(); break;
        case WIFI_SETUP: nextState = StateTickWifiSetup(); break;
        case CHECK_OWNTONE: nextState = StateTickCheckOwntone(); break;
        case QUEUING: nextState = StateTickQueueing(); break;
        case WAITING_FOR_QUEUE: nextState = StateTickWaitingForQueue(); break;
        case SPINNING: nextState = StateTickSpinning(); break;
        case QUEUED: nextState = StateTickQueued(); break;
        case PLAYING: nextState = StateTickPlaying(); break;
        case PAUSED: nextState = StateTickPaused(); break;
        case FINISHED: nextState = StateTickFinished(); break;
        case DONE: nextState = StateTickDone(); break;
        case DRAINING: nextState = StateTickDraining(); break;
        case REBOOT: nextState = StateTickReboot(); break;
        case ERROR: nextState = StateTickError(); break;
        case TAG_ERROR: nextState = StateTickError(); break;
        case QUEUE_ERROR: nextState = StateTickError(); break;
        case OTA_ERROR: nextState = StateTickError(); break;
        case HAL_ERROR: nextState = StateTickError(); break;
        case OFF: nextState = StateTickOff(); break;
  }
  // Check for a HAL error and aren't already in an error state
  if (HALErrorTriggered() && currentState < ERROR) {
    nextState = HAL_ERROR;
  }
  
  // If we aren't transitioning to a new state, abort here
  if (nextState == currentState) return;
  std::cout << "[state] " << magic_enum::enum_name(currentState) << " -> " << magic_enum::enum_name(nextState) << std::endl;
  // Handle the transition to a new state
  switch (nextState) {
      case INIT: break;
      case WAITING_FOR_WIFI: StateStartWaitingForWifi(); break;
      case CHECK_OTA: StateStartCheckOTA(); break;
      case CHECK_POWERSWITCH: StateTickInit(); break;
      case CHECK_OWNTONE: StateStartCheckOwnTone(); break;
      case WAITING_FOR_QUEUE: StateStartWaitingForQueue(); break;
      case SPINNING: StateStartSpinning(); break;
      case OTA: StateStartOta(); break;
      case QUEUING: StateStartQueuing(); break;
      case QUEUED: break;
      case PLAYING: StateStartPlaying(); break;
      case PAUSED: StateStartPaused(); break;
      case FINISHED: StateStartFinished(); break;
      case DONE: StateStartDone(); break;
      case DRAINING: StateStartDraining(); break;
      case OFF: StateStartOff(); break;
      case REBOOT: break;
      case TAG_ERROR: StateStartTagError(); break;
      case QUEUE_ERROR: StateStartQueueError(); break;
      case OTA_ERROR: StateStartOTAError(); break;
      case HAL_ERROR: StateStartHalError(); break;
      case ERROR: break;
      case WIFI_SETUP: break;
  }
  currentState = nextState;
  return;
}

vyknoll_states_t GetState() {
  return currentState;
}