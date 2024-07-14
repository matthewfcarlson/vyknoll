// The HAL for emscripten
#include <vyknoll_hal.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include <iostream>
#include <emscripten.h>
#include <emscripten/html5.h>
#include <GLES2/gl2.h>
#include <emscripten_webgl.h>

#define TFT_HEIGHT           180
#define TFT_WIDTH            640

vyknoll_persisted_config_t _config = {
    .magic = CONFIG_MAGIC,
    .ssid = "ssid",
    .password = "password",
    .server = "localhost",
    .port = 3689
};


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

// LCD Display
bool LcdTurnOff(void) {
    emscripten_run_script(R"(
            (function() {
                var status = document.getElementById('screenstatus');
                if (status) {
                    status.innerHTML = "Screen OFF";
                }
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
            })()
        )");
    return true;
}
bool LcdClearDisplay(void) {
    clear_screen(0.5,0,0,1.0);
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
                    status.innerHTML = "Spin stopped";
                }
            })()
        )");
        return true;
    }
    emscripten_run_script(R"(
        (function() {
            var status = document.getElementById('spinstatus');
            if (status) {
                status.innerHTML = "Spinning";
            }
        })()
    )");
    return true;
}

// NFC reader
bool NFCReadTag(char* tag_data, uint16_t tag_data_len) {
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

void setup(void);
void loop(void);

int main() {
    printf("Starting Vyknoll\n");
    setup();
    emscripten_set_main_loop(loop, 0, true);
    return 0;
}