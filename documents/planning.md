# Vyknoll Reboot — Project Planning Document

*Date: March 2026*

## 1. Vision

Vyknoll is a **physical music player** that brings back the tactile joy of picking a record. Small 7" vinyl-shaped discs with NFC stickers act as the "records." Place one on the device, and it triggers music playback on your HomePods through **Home Assistant** and its **Music Assistant** integration.

The 4" touchscreen ESP32 display serves as the control surface — pick which room to play in, adjust volume, and manage the device's setup. No apps, no phone, no scrolling. **One touch, music plays.**

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

**NFC Tag Strategy: UID → Lookup in Home Assistant**

Each NFC tag only needs its unique ID (UID) — no special data written to the tag. The ESP32 reads the UID and sends it to Home Assistant, which maps it to an album, playlist, or action.

Benefits:
- Tags are dirt cheap and require no programming
- Mappings live in HA (editable via the HA UI, no firmware changes needed)
- Changing what a tag plays doesn't require touching the tag
- The same tag could trigger different behavior based on context (time of day, room, etc.)

Implementation: HA automations triggered by a `tag_scanned` event, or a lookup in a HA helper/input_select entity.

**Speaker/Room Selection: Room-Based**

The device maintains a "current room" selection. When a tag is scanned, music plays in that room. The touchscreen UI lets you switch rooms. Rooms map to HA media player entities (e.g., `media_player.kitchen_homepod`, `media_player.living_room`).

---

## 3. Hardware

### ESP32 Display Module

**Target:** 4" ESP32-S3 touch display module (exact model TBD — needs to be confirmed).

Key requirements for the display module:
- 4" LCD with capacitive touch
- ESP32-S3 (for sufficient RAM/flash for LVGL UI)
- SPI or parallel display interface
- Available GPIO pins for NFC reader connection
- USB-C power (always powered)

**Action item:** Confirm exact board model, display controller IC, and touch controller IC. This determines driver configuration.

### NFC Reader

Options (in order of recommendation):
1. **PN532** — Most common, well-supported, SPI/I2C/UART. Reads most NFC tag types.
2. **RC522** — Cheaper, SPI only, MIFARE-focused. Less flexible but adequate for UID reading.

**Recommendation:** PN532 via I2C (fewer pins used, leaves SPI for display).

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
| TFT_eSPI or LovyanGFX | Display driver (depends on board) |
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
   - Send tag UID to Home Assistant (fire event or call service)
   - Visual/audio feedback on successful scan

3. **Speaker/Room Selection**
   - Fetch list of available media players from HA
   - Display room picker on touchscreen
   - Persist selected room

4. **Basic Playback Controls** *(stretch goal for MVP)*
   - Play/pause button on screen
   - Volume slider
   - Current track name (polled from HA)

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
4. **Now Playing** *(stretch)* — Track info, play/pause, volume

---

## 6. Home Assistant Integration Details

### Authentication

Use a **Long-Lived Access Token** (generated in HA user profile). Sent as a Bearer token in HTTP headers.

```
Authorization: Bearer <LONG_LIVED_ACCESS_TOKEN>
```

### Key API Calls (REST)

**1. Fire a tag scanned event:**
```
POST /api/events/vyknoll_tag_scanned
{
  "tag_uid": "04:A2:B3:C4:D5:E6:F7",
  "room": "media_player.kitchen_homepod"
}
```

HA automation listens for this event and triggers Music Assistant to play the mapped content.

**2. Get available media players:**
```
GET /api/states
```
Filter for entities matching `media_player.*` to build the room picker.

**3. Playback control:**
```
POST /api/services/media_player/media_play_pause
{ "entity_id": "media_player.kitchen_homepod" }

POST /api/services/media_player/volume_set
{ "entity_id": "media_player.kitchen_homepod", "volume_level": 0.5 }
```

**4. Get current playback state:**
```
GET /api/states/media_player.kitchen_homepod
```
Returns `media_title`, `media_artist`, `media_content_id`, `volume_level`, `state` (playing/paused/idle), etc.

### HA Automation Example (Tag → Music)

```yaml
automation:
  - alias: "Vyknoll: Play album from NFC tag"
    trigger:
      - platform: event
        event_type: vyknoll_tag_scanned
    action:
      - choose:
          - conditions:
              - "{{ trigger.event.data.tag_uid == '04:A2:B3:C4:D5:E6:F7' }}"
            sequence:
              - service: mass.play_media  # Music Assistant
                target:
                  entity_id: "{{ trigger.event.data.room }}"
                data:
                  media_id: "Abbey Road"
                  media_type: "album"
          - conditions:
              - "{{ trigger.event.data.tag_uid == '04:B2:C3:D4:E5:F6:A7' }}"
            sequence:
              - service: mass.play_media
                target:
                  entity_id: "{{ trigger.event.data.room }}"
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
- [ ] Wire NFC scan → HA event firing
- [ ] Implement room picker (fetch media players from HA, display as list)
- [ ] Create the HA automation for tag → music (at least 2-3 test tags)
- [ ] Add scan feedback (screen flash, sound if buzzer available)
- [ ] Persist config (WiFi creds, HA token, selected room) to NVS/SPIFFS
- [ ] WiFi setup screen (on-device keyboard for SSID/password)

### Phase 3: Polish (Week 4-6)
- [ ] Now-playing screen (track info, album art if feasible)
- [ ] Playback controls (play/pause, skip, volume slider)
- [ ] Multiple room selection (play in multiple rooms simultaneously)
- [ ] OTA firmware updates
- [ ] Error handling and recovery (WiFi drops, HA unreachable, NFC read failures)
- [ ] UI polish (animations, transitions, consistent theme)

### Phase 4: Physical Design
- [ ] Design and produce mini 7" vinyl discs (laser cut, 3D print, or cardboard)
- [ ] NFC sticker placement on discs
- [ ] Enclosure for ESP32 + NFC reader
- [ ] Tag management UI or workflow (how to add new albums to new tags)

---

## 8. Open Questions

1. **Exact board model** — Need to confirm the 4" ESP32 display module specs (display driver, touch controller, pin mapping). This is a prerequisite for Phase 0.

2. **Music Assistant vs. native HA media** — Are you using the Music Assistant add-on, or just native HA media player integrations? This affects the service calls.

3. **Tag management** — How do you want to add new tags? Options:
   - Manually create HA automations per tag (simple, full control)
   - Build a tag registration UI on the device or in a companion web app
   - Use HA's built-in tag integration (HA can natively handle NFC tag events)

4. **Album art** — Do you want album art on the 4" display? It's achievable but adds complexity (downloading images, decoding JPEG, display memory). Worth considering for Phase 3.

5. **Web simulator** — The original project had an Emscripten web simulator. Is this still valuable for development, or is hardware-first fine?

6. **Project name** — Is "Vyknoll" still the right name, or does the pivot warrant a rename?

---

## 9. File Structure (Proposed)

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
│   │   │   ├── screen_playing.cpp# Now playing screen
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

## 10. Summary

| Aspect | Decision |
|--------|----------|
| **Concept** | NFC-tagged mini vinyl records → music on HomePods |
| **Backend** | Home Assistant + Music Assistant |
| **Communication** | REST API (MVP), WebSocket (later) |
| **Tag strategy** | UID-only tags, mapping lives in HA automations |
| **Hardware** | 4" ESP32-S3 touch display + PN532 NFC reader |
| **Framework** | Arduino/PlatformIO + LVGL |
| **Speaker model** | Room-based selection via touchscreen |
| **Power** | Always plugged in (USB/wall) |
| **MVP scope** | NFC scan → play + room picker + WiFi/HA setup |
