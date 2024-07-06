#include <vyknoll.h>
#include <vyknoll_hal.h>
#include <string.h>

vyknoll_states_t currentState = INIT;
vyknoll_persisted_config_t config = {0};

// Tick functions

vyknoll_states_t StateTickInit() {
    if (!PersistedStateGet(&config)){
        return WIFI_SETUP;
    }
    if (config.magic == 0 || strnlen(config.ssid, 64) == 0 || strnlen(config.password, 64) == 0 || strnlen(config.server, 16) == 0 || config.port == 0) {
        return WIFI_SETUP;
    }
    return WAITING_FOR_WIFI;
}

vyknoll_states_t StateTickWaitingForWifi() {
    // if (WiFi.status() == WL_CONNECTED) {
    //     return CHECK_OTA;
    // }
    return WAITING_FOR_WIFI;
}

vyknoll_states_t StateTickCheckOTA() {
    return CHECK_OWNTONE;
}

vyknoll_states_t StateTickCheckOwntone() {
    // Wait for Owntone to reply with its queue information
    // TODO
    return WAITING_FOR_QUEUE;
}

vyknoll_states_t StateTickCheckPowerswitch() {
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
    // TODO
    return WAITING_FOR_QUEUE;
    // If the queue is empty
    // return SPINNING;
}

vyknoll_states_t StateTickSpinning() {
    // Wait for a tag to be read
    // TODO
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
    return CHECK_POWERSWITCH;
}

// Transition functions
void StateStartInit() {
    return;
}
void StateStartWaitingForWifi() {
    // Connect to wifi with the creds we have stored
    // WiFi.begin(config.ssid, config.password);
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
    // TODO
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
    // TODO
    return;
}
void StateStartTagError() {
    // Display the tag error screen
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
        case CHECK_POWERSWITCH: nextState = StateTickCheckPowerswitch(); break;
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
        case OFF: nextState = StateTickOff(); break;
  }
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
      case QUEUE_ERROR: StateStartTagError(); break;
      case OTA_ERROR: StateStartTagError(); break;
      case ERROR: break;
      case WIFI_SETUP: break;
  }
  currentState = nextState;
  return;
}

vyknoll_states_t GetState() {
  return currentState;
}