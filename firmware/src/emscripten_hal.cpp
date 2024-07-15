// The HAL for emscripten
#include <vyknoll_hal.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include <iostream>
#include <strings.h>
#include <GLES2/gl2.h>
#include <emscripten.h>
#include <emscripten/html5.h>
#include <emscripten_webgl.h>
#include <emscripten/fetch.h>
#include <unordered_map>

#define TFT_HEIGHT           180
#define TFT_WIDTH            640

vyknoll_persisted_config_t _config = {
    .magic = CONFIG_MAGIC,
    .ssid = "ssid",
    .password = "password",
    .server = "localhost",
    .port = 3689
};
std::unordered_map<uint32_t, api_callback_t> webCallbackMap;

void HALSetup() {
    init_webgl(TFT_WIDTH, TFT_HEIGHT);
}

// Read the config from a file
bool PersistedStateGet(vyknoll_persisted_config_t* config) {
    assert(config != nullptr);
    memcpy(config, &_config, sizeof(vyknoll_persisted_config_t));
    return true;
}
// Write the config to a file
bool PersistedStateSave(vyknoll_persisted_config_t* config) {
    assert(config != nullptr);
    memcpy(&_config,config, sizeof(vyknoll_persisted_config_t));
    return true;
}

void SystemReboot() {
    emscripten_run_script("console.error('Rebooting...'); window.location.replace('?q=60');");
}

// LCD Display
bool LcdTurnOff(void) {
    LcdClearDisplay();
    emscripten_run_script(R"(
            (function() {
                var status = document.getElementById('screenstatus');
                if (status) status.innerHTML = "Screen OFF";
                var canvas = document.getElementById('canvas');
                if (canvas) canvas.style.display = 'none';
            })()
        )");
    return true;
}
bool LcdTurnOn(void) {
    emscripten_run_script(R"(
            (function() {
                var status = document.getElementById('screenstatus');
                if (status) {
                    status.innerHTML = "Screen ON";
                }
                var canvas = document.getElementById('canvas');
                if (canvas) canvas.style.display = 'block';
            })()
        )");
    return true;
}
bool LcdClearDisplay(void) {
    clear_screen(0,0,0,1.0);
    return true;
}
bool LcdClearDisplay(uint16_t color) {
    uint16_t scaling = 255 / 32;
    float red = (color >> 11 & 0x1F) / 32.0; // red = 5 bits
    float green = ((color >> 5) & 0x3F) / 64.0; // green = 6 bits
    float blue = (color & 0x1F) / 32.0; // blue = 5 bits
    clear_screen(red,green,blue,1.0);
    return true;
}
bool LcdDrawText(const char* text) {
    return LcdDrawText(text, TFT_HEIGHT/2);
}
bool LcdDrawText(const char* text, uint16_t y) {
    return LcdDrawText(text, TFT_WIDTH/2, y);
}
bool LcdDrawText(const char* text, uint16_t x, uint16_t y) {
    int fontSize = 26;
    float new_y = (float)(TFT_HEIGHT - y - fontSize);
    std::cout << "LCD: " << text << std::endl;
    fill_text(x, new_y, 1.0, 1.0, 1.0, 1.0, text, 0, fontSize, 0);
    return true;

}

// Power Switch
bool PowerSwitchReadIfOn(void) {
    const char* script = R"(
        (function() {
            var checkbox = document.getElementById('powerswitch');
            if (checkbox) {
                return checkbox.checked ? 1 : 0;
            }
            return -1; // Indicate that the checkbox was not found
        })()
    )";
    int result = emscripten_run_script_int(script);
    if (result == 1) {
        return true;
    } else if (result == 0) {
        return false;
    } else {
        std::cout << "Power switch checkbox not found" << std::endl;
    }
    return false;
}
// Needle Switch
bool NeedleSwitchReadIfOn(void) {
    const char* script = R"(
        (function() {
            var checkbox = document.getElementById('needleswitch');
            if (checkbox) {
                return checkbox.checked ? 1 : 0;
            } else { console.log('needleswitch not found'); }
            return -1; // Indicate that the checkbox was not found
        })()
    )";
    int result = emscripten_run_script_int(script);
    if (result == 1) {
        return true;
    } else if (result == 0) {
        return false;
    } else {
        std::cout << "needle switch checkbox not found" << std::endl;
    }
    return false;
}

// turntable
bool TurntableSetSpeed(uint8_t speed) {
    if (speed == 0) {
        emscripten_run_script(R"(
            (function() {
                var status = document.getElementById('spinstatus');
                if (status) {
                    status.innerHTML = 'Spin stopped';
                } else { console.log('spinstatus not found'); }
            })()
        )");
        return true;
    }
    emscripten_run_script(R"(
        (function() {
            var status = document.getElementById('spinstatus');
            if (status) {
                status.innerHTML = 'Spinning';
            } else { console.log('spinstatus not found'); }
        })()
    )");
    return true;
}

// NFC reader
bool NFCReadTag(char* tag_data, uint16_t tag_data_len) {
    char* input_data = emscripten_run_script_string(R"(
        (function() {
            var nfc = document.getElementById('nfctag');
            if (nfc) {
                return nfc.value;
            }
            return '';
        })()
    )");
    strncpy(tag_data, input_data, tag_data_len);
    if (strlen(tag_data) > 0) {
        return true;
    }
    return false;
}

// WiFi
static bool _wifiConnected = false;
bool WiFiConnect(char* ssid, char* password){
    _wifiConnected = true;
    return true;
}
bool WiFiIsConnected(void) {
    return _wifiConnected;
}

typedef struct {
    char* body;
    api_callback_t callback;
} user_data_t;

void downloadSucceeded(emscripten_fetch_t *fetch) {
  user_data_t* userData = (user_data_t*)fetch->userData;
  userData->callback(fetch->status, fetch->data);
  if (userData->body) {
    free((void*)userData->body);
  }
  free((void*)userData);
  emscripten_fetch_close(fetch); // Free data associated with the fetch.
}

void downloadFailed(emscripten_fetch_t *fetch) {
  user_data_t* userData = (user_data_t*)fetch->userData;
  userData->callback(fetch->status, fetch->data);
  if (userData->body) {
    free((void*)userData->body);
  }
  free((void*)userData);
  emscripten_fetch_close(fetch); // Also free data on failure.
}

bool NonBodyRequest(const char* url, api_callback_t callback, const char* method) {
    emscripten_fetch_attr_t attr;
    emscripten_fetch_attr_init(&attr);
    strcpy(attr.requestMethod, method);

    user_data_t* userData = (user_data_t*)malloc(sizeof(user_data_t));
    userData->body = nullptr;
    userData->callback = callback;
    attr.userData = (void*)userData;

    attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
    attr.onsuccess = downloadSucceeded;
    attr.onerror = downloadFailed;
    emscripten_fetch(&attr, url);
    return true;
}

bool GetRequest(const char* url, api_callback_t callback) {
    return NonBodyRequest(url, callback, "GET");
}

bool PutRequest(const char* url, api_callback_t callback) {
    return NonBodyRequest(url, callback, "PUT");
}

bool PostRequest(const char* url, const char* body, api_callback_t callback) {
    emscripten_fetch_attr_t attr;
    emscripten_fetch_attr_init(&attr);
    strcpy(attr.requestMethod, "POST");

    user_data_t* userData = (user_data_t*)malloc(sizeof(user_data_t));
    uint32_t bodyLen = strlen(body) + 1;
    userData->body = (char*)malloc(bodyLen);
    strncpy(userData->body, body, bodyLen);
    userData->callback = callback;
    attr.userData = (void*)userData;
    attr.requestData = body;
    attr.requestDataSize = bodyLen;

    attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
    attr.onsuccess = downloadSucceeded;
    attr.onerror = downloadFailed;
    emscripten_fetch(&attr, url);
    return true;
}

void setup(void);
void loop(void);

int main() {
    printf("Starting Vyknoll\n");
    setup();
    emscripten_set_main_loop(loop, 0, true);
    return 0;
}