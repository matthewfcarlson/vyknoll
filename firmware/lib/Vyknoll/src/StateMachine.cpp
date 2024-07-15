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
#include <ArduinoJson.h>

#define MAX_OUTPUTS 4

vyknoll_states_t currentState = INIT;
vyknoll_persisted_config_t config = {0};

// Internal state
char tagData[64] = {0};
struct {
    char id[32];
    char artist[32];
    char name[32];
    bool notFound;
} nfcTagAlbum = {0};
// TODO: refactor synced properties to be more complex
// Have a local poll method
// Have a local set value method
// Have a server poll method
// Have a server set value method
// Have a client side debounce (the local value must be x older than old value to win)
// Have a server side debounce (the server value must be x older than local value to win)
// Sends update to server if client value is valid and keeps track of when it last sent an update
// Keep alive doesn't need to be a synced property
// Perhaps we need a new type TimestampedProperty which works like a synced property but only has the server side
// Keeps track of when it was last updated
SyncedProperty<bool> ownToneKeepAlive = SyncedProperty<bool>(false);
// TODO: don't use two bools, use an enum
SyncedProperty<bool> ownTonePlaying = SyncedProperty<bool>(false);
SyncedProperty<bool> ownTonePaused = SyncedProperty<bool>(false);
SyncedProperty<uint32_t> ownToneVolume = SyncedProperty<uint32_t>(0);
// Queue length doesn't need to be a synced property
SyncedProperty<uint32_t> ownToneQueueLength = SyncedProperty<uint32_t>(100); // start with a queue size of 100 just because
PeriodicTask* checkOwnToneQueueLength;
PeriodicTask* checkOwnToneVolume;
PeriodicTask* checkOwnToneAlive;
PeriodicTask* checkOwnTonePlayingOrPaused;
TimeOut WifiTimeout = TimeOut(30000);
TimeOut QueueTimeout = TimeOut(30000);
TimeOut TagReadTimeout = TimeOut(60000);
TimeOut QueryTimeout = TimeOut(45000);
char* outputNames[MAX_OUTPUTS] = {0};
bool outputsEnabled[MAX_OUTPUTS] = {0};
static bool _halErrorTriggered = false;

void InternalReset() {
    bzero(tagData, sizeof(tagData));
    bzero(&nfcTagAlbum, sizeof(nfcTagAlbum));
    bzero(outputsEnabled, sizeof(outputsEnabled));
    for (int i = 0; i < MAX_OUTPUTS; i++) {
        if (outputNames[i] == nullptr) continue;
        free(outputNames[i]);
        outputNames[i] = nullptr;
    }
    ownToneKeepAlive.Reset();
    ownToneVolume.Reset();
    ownToneQueueLength.Reset();
}

// Hal function
void HALTriggerError(void) {
    _halErrorTriggered = true;
}
bool HALErrorTriggered(void) {
    return _halErrorTriggered;
}

void CreateServerURL(char* url, uint32_t url_size, const char* endpoint) {
    snprintf(url, url_size, "http://%s:%d%s", config.server, config.port, endpoint);
}

// Tick functions
void StateMachineSetup() {
   checkOwnToneAlive = new PeriodicTask(1000, [](){
        char url[128] = {0};
        CreateServerURL(url, sizeof(url), "/api/config");
        // Check if the server is alive
        GetRequest(url, [](uint16_t code, const char* data){
            if (code == 200) {
                printf("Got response from server. Set to alive\n");
                ownToneKeepAlive.UpdateServer(true);
                return;
            }
            ownToneKeepAlive.UpdateServer(false);
        });
   });
   checkOwnToneQueueLength = new PeriodicTask(1000, [](){
        char url[128] = {0};
        CreateServerURL(url, sizeof(url), "/api/queue");
        // Check if the server is alive
        GetRequest(url, [](uint16_t code, const char* json){
            if (code == 200) {
                JsonDocument doc;
                DeserializationError error = deserializeJson(doc, json);
                if (error) {
                    printf("Failed to parse JSON\n");
                    return;
                }
                uint32_t queueLength = doc["count"];
                if (ownToneQueueLength.GetServer().value != queueLength) printf("Got queue length: %d\n", queueLength);
                ownToneQueueLength.UpdateServer(queueLength);
                return;
            }
        });
   });
   checkOwnTonePlayingOrPaused = new PeriodicTask(1000, []() {
        char url[128] = {0};
        CreateServerURL(url, sizeof(url), "/api/player");
        // Check if the server is alive
        GetRequest(url, [](uint16_t code, const char* json){
            if (code == 200) {
                JsonDocument doc;
                DeserializationError error = deserializeJson(doc, json);
                if (error) {
                    printf("Failed to parse JSON\n");
                    return;
                }
                const char* state = doc["state"];
                printf("Got State: %s\n", state);
                if (strncmp(state, "play", 4) == 0) {
                    ownTonePlaying.UpdateServer(true);
                    ownTonePaused.UpdateServer(false);
                    return;
                }
                if (strncmp(state, "pause", 5) == 0) {
                    ownTonePlaying.UpdateServer(false);
                    ownTonePaused.UpdateServer(true);
                    return;
                }
                ownTonePlaying.UpdateServer(false);
                ownTonePaused.UpdateServer(false);
                return;
            }
        });
   });
}

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
    assert(checkOwnToneAlive != nullptr);
    checkOwnToneAlive->Check();
    assert(checkOwnToneQueueLength != nullptr);
    checkOwnToneQueueLength->Check();
    if (ownToneKeepAlive.IsValid() && ownToneKeepAlive.GetValue().value == false) {
        return CHECK_OWNTONE;
    }
    if (ownToneQueueLength.IsValid() && ownToneQueueLength.GetValue().value == 0) {
        return SPINNING;
    }
    return WAITING_FOR_QUEUE;
}

vyknoll_states_t StateTickCheckPowerSwitch() {
    TurntableSetSpeed(0);
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
    if (ownToneQueueLength.IsValid() && ownToneQueueLength.GetValue().value == 0) {
        return SPINNING;
    }
    if (QueueTimeout.Check()) {
        printf("[ERROR:%s:%d] Queue timeout\n", __FUNCTION__, __LINE__);
        return QUEUE_ERROR;
    }
    // Check if a button is pressed, do a drain then
    // if (NeedleSwitchReadIfOn()) {
    //     return DRAINING;
    // }
    if (!PowerSwitchReadIfOn()) {
        return OFF;
    }
    return WAITING_FOR_QUEUE;
}

vyknoll_states_t StateTickSpinning() {
    // Wait for a tag to be read
    if (NFCReadTag(tagData, sizeof(tagData))) {
        // We got a tag, move on
        // TODO: do we need to issue a lookup first?
        printf("[NFC] Found tag: %s\n", tagData);
        bzero(&nfcTagAlbum, sizeof(nfcTagAlbum));
        char url[256] = {0};
        CreateServerURL(url, sizeof(url), "/api/search?type=albums&media_kind=music&query=");
        strncat(url, tagData, sizeof(url) - strlen(url) - 1);
        GetRequest(url, [](uint16_t code, const char* json){
            if (code == 200) {
                JsonDocument doc;
                DeserializationError error = deserializeJson(doc, json);
                if (error) {
                    printf("[NFC] Query - Failed to parse JSON: %s\n",json);
                    HALTriggerError();
                    return;
                }
                // TODO: implement debug print
                // printf("[NFC] Query - Got response:");
                // serializeJson(doc, std::cout);
                // printf("\n");
                if (!doc.containsKey("albums") || !doc["albums"].containsKey("items")) {
                    printf("[NFC] Query - No items key found\n");
                    HALTriggerError();
                    return;
                }
                uint32_t returnCount = doc["albums"]["total"];
                if (returnCount == 0) {
                    printf("[NFC] Query - No results found: %d\n", returnCount);
                    nfcTagAlbum.notFound = true;
                    return;
                }
                if (returnCount > 1) {
                    // TODO: be able to handle multiple results
                    printf("[NFC] Query - Multiple results found\n");
                    HALTriggerError();
                    return;
                }
                const char* id = doc["albums"]["items"][0]["id"] | "unknown";
                const char* name = doc["albums"]["items"][0]["name"] | "unknown";
                const char* artist = doc["albums"]["items"][0]["artist"] | "unknown";
                strncpy(nfcTagAlbum.id, id, sizeof(nfcTagAlbum.id));
                strncpy(nfcTagAlbum.name, name, sizeof(nfcTagAlbum.name));
                strncpy(nfcTagAlbum.artist, artist, sizeof(nfcTagAlbum.artist));
                printf("[NFC] Album: %s %s %s\n", nfcTagAlbum.id, nfcTagAlbum.name, nfcTagAlbum.artist);
                return;
            } else {
                // We should trigger a not found error
                HALTriggerError();
            }
            return;
        });
        // How are NFC tags encoded?
        return QUERYING;
    }
    if (TagReadTimeout.Check()) {
        printf("[ERROR:%s:%d] Tag read timeout\n", __FUNCTION__, __LINE__);
        return TAG_ERROR;
    }
    if (!PowerSwitchReadIfOn()) {
        return OFF;
    }
    return SPINNING;
}
vyknoll_states_t StateTickQuerying() {
    if (QueryTimeout.Check()) {
        printf("[ERROR:%s:%d] Query timeout\n", __FUNCTION__, __LINE__);
        return TAG_ERROR;
    }
    if (nfcTagAlbum.notFound) {
        return TAG_ERROR;
    }
    if (nfcTagAlbum.id[0] == 0) {
        return QUERYING;
    }
    return QUEUING;
}
vyknoll_states_t StateTickQueueing() {
    checkOwnToneQueueLength->Check();
    if (QueueTimeout.Check()) {
        printf("[ERROR:%s:%d] Queue timeout\n", __FUNCTION__, __LINE__);
        return QUEUE_ERROR;
    }
    // Wait for the Owntone queue to have some items in it
    if (ownToneQueueLength.IsValid() && ownToneQueueLength.GetValue().value > 0) {
        return QUEUED;
    }
    // If they try to put the needle down, it's an error
    if (NeedleSwitchReadIfOn()) {
        printf("[ERROR:%s:%d] Needle down before queued\n", __FUNCTION__, __LINE__);
        return QUEUE_ERROR;
    }
    return QUEUING;
}
vyknoll_states_t StateTickQueued() {
    // If we don't have anything in the queue, we are error
    checkOwnToneQueueLength->Check();
    if (ownToneQueueLength.IsValid() && ownToneQueueLength.GetValue().value == 0) {
        printf("[ERROR:%s:%d] Empty Queue\n", __FUNCTION__, __LINE__);
        return QUEUE_ERROR;
    }
    // Check if the power switch is off
    if (!PowerSwitchReadIfOn()) {
        return DRAINING;
    }
    // Check if the needle is on
    if (NeedleSwitchReadIfOn()) {
        return PLAY;
    }
    return QUEUED;
}

vyknoll_states_t StateTickPlay() {
    checkOwnTonePlayingOrPaused->Check();
    // If the needle is on but we're here, we must have been played by the server
    if (!NeedleSwitchReadIfOn()) return PAUSE;
    if (ownTonePlaying.GetValue().value) return PLAYING;
    return PLAY;
}

vyknoll_states_t StateTickPlaying() {
    checkOwnToneQueueLength->Check();
    if (ownToneQueueLength.IsValid() && ownToneQueueLength.GetValue().value == 0) {
        return FINISHED;
    }
    checkOwnTonePlayingOrPaused->Check();
    if (ownTonePaused.IsValid() && ownTonePaused.GetValue().value) {
        auto value = ownTonePaused.GetValue();
        printf("[PLAYING] Server says it's paused. Paused = %x, server=%x, mills = %lld\n", value.value, value.server, value.millsSinceUpdate);
        return PAUSE_EXTERN;
    }
    // Check if the power switch is off
    if (!PowerSwitchReadIfOn()) {
        return DRAINING;
    }
    // Check if the needle is off
    if (!NeedleSwitchReadIfOn()) {
        return PAUSE;
    }
    return PLAYING;
}

vyknoll_states_t StateTickPause() {
    checkOwnTonePlayingOrPaused->Check();
    // If the needle is on but we're here, we must have been paused by the server
    if (NeedleSwitchReadIfOn()) return PAUSE;
    if (ownTonePaused.GetValue().value) return PAUSED;
    return PAUSE;
}

vyknoll_states_t StateTickPaused() {
    checkOwnToneQueueLength->Check();
    if (ownToneQueueLength.IsValid() && ownToneQueueLength.GetValue().value == 0) {
        return FINISHED;
    }
    checkOwnTonePlayingOrPaused->Check();
    if (ownTonePlaying.IsValid() && ownTonePlaying.GetValue().value) {
        auto value = ownTonePlaying.GetValue();
        printf("[PAUSED] Server says it's playing. ownTonePlaying = %x, server=%x, mills = %lld\n", value.value, value.server, value.millsSinceUpdate);
        return PLAY_EXTERN;
    }
    // Check if the power switch is off
    if (!PowerSwitchReadIfOn()) {
        return DRAINING;
    }
    // Check if the needle is on
    if (NeedleSwitchReadIfOn()) {
        return PLAY;
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
    checkOwnToneQueueLength->Check();
    if (ownToneQueueLength.IsValid() && ownToneQueueLength.GetValue().value == 0) {
        return OFF;
    }
    return DRAINING;
}

vyknoll_states_t StateTickReboot() {
    // Reboot the device
    LcdClearDisplay();
    TurntableSetSpeed(0);
    SystemReboot();
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
    LcdTurnOn();
    LcdClearDisplay();
    // Turn on the turntable
    TurntableSetSpeed(45);
    TagReadTimeout.Start();
    return;
}
void StateStartOta() {
    // Display the OTA Graphic
    // TODO
}
void StateStartQuerying() {
    QueryTimeout.Start();
    LcdTurnOn();
    LcdClearDisplay();
    LcdDrawText("Searching");
    return;
}
void StateStartQueuing() {
    QueueTimeout.Start();
    LcdTurnOn();
    LcdClearDisplay();
    LcdDrawText(nfcTagAlbum.name, 30);
    LcdDrawText(nfcTagAlbum.artist, 70);
    char url[256] = {0};
    CreateServerURL(url, sizeof(url), "/api/queue/items/add?uris=library:album:");
    strncat(url, nfcTagAlbum.id, sizeof(url) - strlen(url) - 1);
    PostRequest(url, "", [](uint16_t code, const char* data){
        if (code == 200) {
            printf("Added to queue\n");
            return;
        }
        printf("Failed to add to queue\n");
        HALTriggerError();
    });
    return;
}
void StateStartPlay() {
    char url[128] = {0};
    CreateServerURL(url, sizeof(url), "/api/player/play");
    ownTonePaused.UpdateLocal(false);
    ownTonePlaying.UpdateLocal(true);
    // Pause
    PutRequest(url, [](uint16_t code, const char* data){
        if (code == 204 || code == 200) return;
        printf("Failed to play: %d. %s\n", code, data);
        HALTriggerError();
    });
    return;
}
void StateStartPlaying() {
    // TODO: Display the playing screen
}
void StateStartPause() {
    char url[128] = {0};
    CreateServerURL(url, sizeof(url), "/api/player/pause");
    ownTonePaused.UpdateLocal(true);
    ownTonePlaying.UpdateLocal(false);
    // Pause
    PutRequest(url, [](uint16_t code, const char* data){
        if (code == 204 || code == 200) return;
        printf("Failed to pause: %d. %s\n", code, data);
        HALTriggerError();
    });
}
void StateStartPaused() {
    // TODO: Display the paused screen
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
    LcdClearDisplay();
    LcdDrawText("Draining Queue");
    char url[128] = {0};
    CreateServerURL(url, sizeof(url), "/api/queue/clear");
    // Start draining the server
    PutRequest(url, [](uint16_t code, const char* data){
        if (code == 204 || code == 200) {
            printf("Queue cleared\n");
            return;
        }
        printf("Failed to clear queue: %d. %s\n", code, data);
        HALTriggerError();
    });
    return;
}
void StateStartOff() {
    // Display the off screen
    LcdTurnOff();
    // Turn off the turntable
    TurntableSetSpeed(0);
    // Reset everything
    InternalReset();
    return;
}
void StateStartTagError() {
    // Display the tag error screen
    LcdTurnOn();
    LcdClearDisplay(COLOR_RED);
    LcdDrawText("NFC Tag Error");
    return;
}
void StateStartOTAError() {
    // Display the ota error screen
    LcdTurnOn();
    LcdClearDisplay(COLOR_RED);
    LcdDrawText("OTA Error");
    return;
}
void StateStartHalError() {
    // Display the hal error screen
    LcdTurnOn();
    LcdClearDisplay(COLOR_RED);
    LcdDrawText("Hardware Error");
    return;
}
void StateStartQueueError() {
    // Display the queue error screen
    LcdTurnOn();
    LcdClearDisplay(COLOR_RED);
    LcdDrawText("Queue Error");
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
        case QUERYING: nextState = StateTickQuerying(); break;
        case QUEUED: nextState = StateTickQueued(); break;
        case PLAY: nextState = StateTickPlay(); break;
        case PLAY_EXTERN: nextState = StateTickPlay(); break;
        case PLAYING: nextState = StateTickPlaying(); break;
        case PAUSE: nextState = StateTickPause(); break;
        case PAUSE_EXTERN: nextState = StateTickPause(); break;
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
    printf("[ERROR:%s:%d] HAL error\n", __FUNCTION__, __LINE__);
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
      case QUERYING: StateStartQuerying(); break;
      case QUEUING: StateStartQueuing(); break;
      case QUEUED: break;
      case PLAY: StateStartPlay(); break;
      case PLAY_EXTERN: StateStartPlaying(); break;
      case PLAYING: StateStartPlaying(); break;
      case PAUSE: StateStartPause(); break;
      case PAUSE_EXTERN: StateStartPlaying(); break;
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