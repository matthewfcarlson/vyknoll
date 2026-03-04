# Vyknoll Reboot — Project Planning Document

*Date: March 2026*

## 1. Vision

Vyknoll is a **physical music player** that brings back the tactile joy of picking a record. Small 7" vinyl-shaped discs — each printed with album art and fitted with an NFC sticker — act as the "records." Place one on the device, and it triggers music playback on your HomePods through **Home Assistant** and its **Music Assistant** add-on.

The 4" touchscreen ESP32 display serves as the control surface — pick which room to play in, see the current track title, adjust volume, and manage the device's setup. The album art lives on the physical disc, not the screen. No apps, no phone, no scrolling. **One touch, music plays.**

### Core Experience

1. Walk up to the Vyknoll device
2. Pick a mini record from your collection (each one has an NFC sticker on it)
3. Place it on/near the device's NFC reader
4. Music starts playing on your selected HomePod(s)
5. Use the touchscreen to switch rooms, adjust volume, or skip tracks

### Who Is It For?

Anyone in the household who wants to play music without fumbling with a phone. Kids who can't navigate Spotify. Guests who don't have your WiFi password. You, when you just want to *put on a record*.

---

## 2. Architecture Overview

```
┌──────────────┐        ┌──────────────────┐        ┌─────────────┐
│  NFC Tag     │        │  Home Assistant   │        │  HomePods   │
│  (UID only)  │──scan──│                  │──play──│  (AirPlay)  │
│              │        │  Music Assistant  │        │             │
└──────────────┘        │  + Automations   │        └─────────────┘
                        └────────▲─────────┘
                                 │
                          REST API / WebSocket
                                 │
                        ┌────────┴─────────┐
                        │  ESP32 Display   │
                        │  (Vyknoll)       │
                        │                  │
                        │  - NFC Reader    │
                        │  - 4" Touch LCD  │
                        │  - WiFi          │
                        └──────────────────┘
```

### Key Architectural Decisions

**ESP32 → Home Assistant Communication: REST API (MVP), WebSocket (later)**

For the MVP, use Home Assistant's [REST API](https://developers.home-assistant.io/docs/api/rest/) to call services and read state. It's simpler to implement, easier to debug, and sufficient for a device that's always powered on.

Later, upgrade to the [WebSocket API](https://developers.home-assistant.io/docs/api/websocket/) for real-time state updates (now-playing info, speaker state changes made from other devices, etc.).

*Why not MQTT?* MQTT adds a broker dependency and isn't natively how HA media players are controlled. REST/WebSocket talks directly to HA's core.

*Why not ESPHome native API?* See the framework recommendation in Section 4.

**NFC Tag Strategy: HA's Built-in Tag Integration**

Each NFC tag only needs its unique ID (UID) — no special data written to the tag. The ESP32 reads the UID and sends it to Home Assistant via the native **Tag Scanned API** (`POST /api/tag/scanned`). HA registers tags automatically and provides a UI for managing them and creating automations.

Benefits:
- Tags are dirt cheap and require no programming
- HA auto-discovers new tags — scan an unknown tag and HA prompts you to name it and set up an automation
- Mappings are managed entirely in HA's UI (no firmware changes needed)
- Changing what a tag plays doesn't require touching the tag
- The same tag could trigger different behavior based on context (time of day, room, etc.)
- Leverages HA's mature, well-tested tag infrastructure — no custom event system needed

Implementation: ESP32 calls `POST /api/tag/scanned` with the tag ID. HA's Tag integration handles registration, and HA automations trigger Music Assistant playback via `tag_scanned` event triggers.

**Speaker/Room Selection: Room-Based**

The device maintains a "current room" selection. When a tag is scanned, music plays in that room. The touchscreen UI lets you switch rooms. Rooms map to HA media player entities (e.g., `media_player.kitchen_homepod`, `media_player.living_room`).

---

## 3. Hardware

### ESP32 Display Module

**Board:** [Makerfabs MaTouch ESP32-S3 Parallel TFT with Touch 4.0"](https://www.makerfabs.com/esp32-s3-parallel-tft-with-touch-4-inch.html)

| Spec | Detail |
|------|--------|
| **Display** | 4" IPS, 480×480, RGB565 parallel interface, >80 FPS |
| **Display driver** | ST7701 (3-wire SPI init + RGB parallel data) |
| **Touch** | 5-point capacitive touch |
| **MCU** | ESP32-S3 (dual-core LX7, up to 240 MHz) |
| **Memory** | OPI PSRAM, 16MB flash |
| **Storage** | MicroSD card slot (onboard 16GB SD) |
| **Connectivity** | WiFi + Bluetooth 5.0 |
| **Connectors** | 2× Mabee/Grove (I2C + GPIO), speaker out, LiPo battery socket |
| **Power** | USB-C 5V |
| **Extras** | Built-in speaker output (3Ω 4W), LiPo charger |

**NFC connection:** PN532 connects via one of the onboard Mabee/Grove I2C connectors — no soldering or custom wiring needed.

**Display driver note:** ST7701 uses a 3-wire SPI bus for initialization commands, then switches to RGB parallel for pixel data. LovyanGFX has ST7701 support. The [Makerfabs GitHub repo](https://github.com/Makerfabs/ESP32-S3-Parallel-TFT-with-Touch-4inch) has example pin configs and driver setup code.

### NFC Reader

Options (in order of recommendation):
1. **PN532** — Most common, well-supported, SPI/I2C/UART. Reads most NFC tag types.
2. **RC522** — Cheaper, SPI only, MIFARE-focused. Less flexible but adequate for UID reading.

**Recommendation:** PN532 via I2C, connected to a Mabee/Grove port on the MaTouch board. Plug-and-play.

### NFC Tags

- **NTAG213** or **NTAG215** stickers — cheap, widely available, reliable
- Only the UID is needed, so even the smallest/cheapest tags work
- Stick them on 7" mini vinyl disc cutouts (cardboard, acrylic, 3D printed, etc.)

---

## 4. Framework & Development Approach

### Recommendation: Arduino/PlatformIO + LVGL

After considering the project requirements, **Arduino/PlatformIO with LVGL** is the recommended approach over ESPHome.

**Why not ESPHome?**
ESPHome is excellent for simple HA devices, but this project has a rich, interactive touch UI with multiple screens (room picker, playback controls, settings). ESPHome's LVGL support works but complex multi-screen UIs in YAML become unwieldy. You lose the ability to write clean, structured C++ logic for UI state management.

**Why Arduino/PlatformIO + LVGL?**
- **LVGL** is a mature, full-featured UI toolkit designed for embedded displays. It handles touchscreens, animations, and complex layouts natively.
- **Full C++ control** for the UI state machine, HTTP communication, and NFC handling.
- **SquareLine Studio** (optional) can be used to visually design the LVGL UI and export C code.
- **PlatformIO** provides solid build, dependency management, and multi-environment support (including a native test target).
- Builds on your existing experience with the original Vyknoll codebase.

**Key Libraries:**
| Library | Purpose |
|---------|---------|
| LVGL | Touch UI framework |
| LovyanGFX | Display driver (ST7701 support, RGB parallel) |
| ArduinoJson | JSON parsing for HA REST API |
| PN532 (Adafruit or similar) | NFC tag reading |
| WiFiManager or custom | WiFi provisioning |
| HTTPClient (built-in) | REST API calls to HA |
| WebSocketsClient | Future: real-time HA state |

### Alternative Considered: ESPHome + LVGL

If you want faster iteration and less C++ code, ESPHome is viable. It provides native HA integration, built-in NFC support, and LVGL display support. The tradeoff is less flexibility for complex UI flows and custom logic. This could be revisited if the Arduino approach proves too heavy.

---

## 5. MVP (Minimum Viable Product)

The MVP proves the end-to-end flow: **scan tag → music plays on HomePod, controllable from the screen.**

### MVP Features

1. **WiFi & HA Setup**
   - Connect to WiFi (captive portal or hardcoded for MVP)
   - Configure HA URL and Long-Lived Access Token
   - Persist configuration to flash

2. **NFC Tag Scanning**
   - Read NFC tag UID when presented
   - Send tag UID to Home Assistant via `POST /api/tag/scanned`
   - HA auto-registers unknown tags for easy setup
   - Visual feedback on successful scan

3. **Speaker/Room Selection**
   - Fetch list of available media players from HA
   - Display room picker on touchscreen
   - Persist selected room

4. **Basic Playback Controls** *(stretch goal for MVP)*
   - Play/pause button on screen
   - Volume slider
   - Current track title (polled from HA)

### MVP Architecture

```
┌─────────────────────────────────────────────┐
│                 ESP32 Firmware               │
│                                              │
│  ┌──────────┐  ┌──────────┐  ┌───────────┐  │
│  │ NFC      │  │ UI       │  │ HA Client │  │
│  │ Reader   │  │ (LVGL)   │  │ (REST)    │  │
│  │          │  │          │  │           │  │
│  │ Read UID ├──► Show     ├──► Call      │  │
│  │          │  │ feedback │  │ service   │  │
│  └──────────┘  └──────────┘  └───────────┘  │
│                                              │
│  ┌──────────────────────────────────────┐    │
│  │ Config Manager (WiFi, HA URL, Token) │    │
│  └──────────────────────────────────────┘    │
└─────────────────────────────────────────────┘
```

### MVP Screens

1. **Setup Screen** — WiFi SSID/password, HA URL, access token input
2. **Home Screen** — "Ready to scan" state, current room displayed, tap to change room
3. **Room Picker** — List of available HA media players, tap to select
4. **Now Playing** *(stretch)* — Track title/artist, play/pause, volume

---

## 6. Home Assistant Integration Details

### Authentication

Use a **Long-Lived Access Token** (generated in HA user profile). Sent as a Bearer token in HTTP headers.

```
Authorization: Bearer <LONG_LIVED_ACCESS_TOKEN>
```

### Key API Calls (REST)

**1. Report a tag scan (HA's native Tag API):**
```
POST /api/tag/scanned
{
  "tag_id": "04:A2:B3:C4:D5:E6:F7",
  "device_id": "vyknoll_device_id"
}
```

HA's built-in Tag integration receives this, registers unknown tags automatically, and fires a `tag_scanned` event that automations can trigger on.

**2. Set the active room (input_select helper):**

Store the currently selected room in an HA `input_select` helper so automations can reference it:
```
POST /api/services/input_select/select_option
{
  "entity_id": "input_select.vyknoll_room",
  "option": "Kitchen HomePod"
}
```

**3. Get available media players:**
```
GET /api/states
```
Filter for entities matching `media_player.*` to build the room picker.

**4. Playback control:**
```
POST /api/services/media_player/media_play_pause
{ "entity_id": "media_player.kitchen_homepod" }

POST /api/services/media_player/volume_set
{ "entity_id": "media_player.kitchen_homepod", "volume_level": 0.5 }
```

**5. Get current playback state:**
```
GET /api/states/media_player.kitchen_homepod
```
Returns `media_title`, `media_artist`, `volume_level`, `state` (playing/paused/idle), etc.

### HA Setup

**Required HA helpers:**
- `input_select.vyknoll_room` — tracks which room/speaker is currently selected on the Vyknoll device

**Tag registration workflow:**
1. Stick an NFC sticker on a new mini vinyl disc
2. Scan it on the Vyknoll device
3. HA detects a new tag and prompts you to name it (e.g., "Abbey Road")
4. Create an automation in HA's UI: when this tag is scanned → call Music Assistant to play the album on the room from `input_select.vyknoll_room`

### HA Automation Example (Tag → Music)

```yaml
automation:
  - alias: "Vyknoll: Abbey Road"
    trigger:
      - platform: tag
        tag_id: "04:A2:B3:C4:D5:E6:F7"
    action:
      - service: mass.play_media  # Music Assistant
        target:
          entity_id: "{{ states('input_select.vyknoll_room') }}"
        data:
          media_id: "Abbey Road"
          media_type: "album"

  - alias: "Vyknoll: Chill Vibes Playlist"
    trigger:
      - platform: tag
        tag_id: "04:B2:C3:D4:E5:F6:A7"
    action:
      - service: mass.play_media
        target:
          entity_id: "{{ states('input_select.vyknoll_room') }}"
        data:
          media_id: "Chill Vibes"
          media_type: "playlist"
```

---

## 7. Implementation Phases

### Phase 0: Hardware Verification
- [ ] Confirm exact ESP32 display board model and specs
- [ ] Get display working (blank screen → colored screen → text)
- [ ] Get touch input working
- [ ] Wire up NFC reader and read a tag UID
- [ ] Confirm HA REST API works from ESP32 (simple GET request)

### Phase 1: Skeleton (Week 1-2)
- [ ] Set up PlatformIO project with LVGL
- [ ] Configure display driver for the specific board
- [ ] Create basic LVGL UI skeleton (two screens: setup + home)
- [ ] Implement WiFi connection (hardcoded credentials for now)
- [ ] Implement HA REST client (authentication + basic GET)
- [ ] Read NFC tag UID and print to serial

### Phase 2: MVP (Week 2-4)
- [ ] Wire NFC scan → HA tag scanned API call
- [ ] Implement room picker (fetch media players from HA, display as list)
- [ ] Create the HA automation for tag → music (at least 2-3 test tags)
- [ ] Add scan feedback (screen flash, sound if buzzer available)
- [ ] Persist config (WiFi creds, HA token, selected room) to NVS/SPIFFS
- [ ] WiFi setup screen (on-device keyboard for SSID/password)

### Phase 3: Polish (Week 4-6)
- [ ] Now-playing screen (track title/artist, playback state)
- [ ] Playback controls (play/pause, skip, volume slider)
- [ ] Multiple room selection (play in multiple rooms simultaneously)
- [ ] OTA firmware updates
- [ ] Error handling and recovery (WiFi drops, HA unreachable, NFC read failures)
- [ ] UI polish (animations, transitions, consistent theme)

### Phase 4: Physical Design
- [ ] Design and produce mini 7" vinyl discs (laser cut, 3D print, or cardboard)
- [ ] NFC sticker placement on discs
- [ ] Enclosure for ESP32 + NFC reader
- [ ] Tag management workflow documentation (scan new tag → HA auto-registers → create automation in HA UI)

---

## 8. Resolved Decisions

| Question | Decision |
|----------|----------|
| Music backend | **Music Assistant add-on** — supports many players and sources |
| Tag management | **HA's built-in Tag integration** — scan a new tag, HA auto-registers it, create automations in HA's UI |
| Album art on display | **No** — album art lives on the physical vinyl disc. Display shows track title/artist only |
| Web simulator | **Removed** — hardware-first development, no Emscripten target |
| Project name | **Vyknoll** — keeping the name |

## 9. Open Questions

All major questions resolved. Hardware confirmed as Makerfabs MaTouch ESP32-S3 4.0".

---

## 10. File Structure (Proposed)

```
vyknoll/
├── firmware/
│   ├── src/
│   │   ├── main.cpp              # Setup + loop
│   │   ├── ha_client.cpp/.h      # Home Assistant REST API client
│   │   ├── nfc_reader.cpp/.h     # NFC tag reading
│   │   ├── ui/
│   │   │   ├── ui.cpp/.h         # LVGL UI manager
│   │   │   ├── screen_home.cpp   # Home/ready screen
│   │   │   ├── screen_rooms.cpp  # Room picker screen
│   │   │   ├── screen_playing.cpp # Now playing (track title, controls)
│   │   │   └── screen_setup.cpp  # WiFi/HA setup screen
│   │   └── config.cpp/.h         # Persistent config (NVS)
│   ├── test/
│   │   └── ...
│   └── platformio.ini
├── ha/
│   └── automations.yaml          # Example HA automations for tag mapping
├── documents/
│   ├── planning.md               # This document
│   └── ...
└── README.md
```

---

## 11. Summary

| Aspect | Decision |
|--------|----------|
| **Concept** | NFC-tagged mini vinyl records → music on HomePods |
| **Backend** | Home Assistant + Music Assistant add-on |
| **Communication** | REST API (MVP), WebSocket (later) |
| **Tag strategy** | UID-only tags, HA built-in Tag integration, automations in HA UI |
| **Hardware** | Makerfabs MaTouch ESP32-S3 4" (480×480, ST7701) + PN532 NFC via I2C |
| **Framework** | Arduino/PlatformIO + LVGL |
| **Speaker model** | Room-based selection via touchscreen |
| **Power** | Always plugged in (USB/wall) |
| **MVP scope** | NFC scan → play + room picker + WiFi/HA setup |
