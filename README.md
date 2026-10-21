# View my other ESP32 Project Repo here
- https://github.com/DanversKara/ESP32-S3-Box-3/tree/main/ESP32/ESP32-S3-Box-3/Firmware%20designed%20FreePBX%20or%20IncrediblePBX%20or%20Asterisk/VoIP%2BApps%2BKeypad-Pin_Code-Settings-Locks

# CCTV Box — ESP32-S3-BOX-3

Standalone 4-camera viewer and SD-card recorder, built with ESPHome + LVGL.

## What it does
- **Home:** clock and a floating camera window. Drag it to move it, drag the amber corner to resize it, tap it to open that camera, X to hide it.
- **CCTV:** one camera full width. **1–4** pick the camera, **REC** records it to the SD card, **FLOAT** pins it to the home window, **GRID** shows all four.
- **Grid:** all four cameras 2×2. Tap one to open it; X goes back.
- **Recording:** `/cctv/<YYYYMMDD_HHMMSS>_<NAME>.avi` on the card (Motion-JPEG AVI, plays in VLC). A new file starts every 5 minutes. The oldest files are deleted when free space drops below 1 GB.
- **Home Assistant:** a *Record <camera>* switch for each camera, *Record All* / *Stop All* buttons, a floating window on/off switch and camera select, and SD free space / size / file count sensors.

Recording keeps working when a camera isn't on screen. It resumes after a reboot (it waits up to 90 s for the clock, because file names use the date).

## Hardware
- ESP32-S3-BOX-3.
- **Recording needs the BOX-3-SENSOR base**, which has the microSD slot. The plain BOX-3-DOCK has no slot; viewing still works without it.
- microSD card formatted **FAT32**. Cards over 32 GB usually come as exFAT, so reformat them as FAT32 (for example with *guiformat* on Windows).

## Cameras
- MJPEG only, over HTTP (motionEye/motion streaming port, ESP32-CAM) or RTSP (camera sub-stream set to MJPEG). H.264/H.265 is not supported.
- Keep streams small, ideally 320×240 to 640×480. 1080p works but is slow to show, and each frame is roughly 100 KB on the card.

## Install (ESPHome add-on in Home Assistant)
1. Copy `cctv-box.yaml` and the whole `components` folder into `/config/esphome/`:
   ```
   esphome/
   ├── cctv-box.yaml
   ├── secrets.yaml            (wifi_ssid, wifi_password, api_key)
   └── components/
       ├── esp_rtsp_stream/
       └── cctv_sdcard/
   ```
2. Edit the `substitutions:` at the top of the YAML (camera URLs, names, frame rates).
3. Install. Use USB for the first flash.

## Settings worth knowing
| Setting | Default | Notes |
|---|---|---|
| `cctv_single_fps` / `cctv_grid_fps` | 4 / 2 | On-screen decode rate per camera |
| `cctv_record_fps` | 2 | Frames written per second, per camera |
| `cctv_record_segment` | 5min | Length of each file |
| `min_free` (cctv_sdcard) | 1GB | Oldest recordings are deleted below this |
| `keep_newest` | 8 | These files are never auto-deleted |

Space used: 1080p at 2 fps is about 0.7 GB per hour per camera; 640×480 at 2 fps is about 0.15 GB per hour.

# adding this concept to my ESP Projects
- found at https://github.com/DanversKara/ESP32-S3-Box-3/tree/main/ESP32/ESP32-S3-Box-3/Firmware%20designed%20FreePBX%20or%20IncrediblePBX%20or%20Asterisk/VoIP%2BApps%2BKeypad-Pin_Code-Settings-Locks now i have working faster video then my old video setup under archive folder.
- also attached a video https://github.com/DanversKara/Esp-rtsp-stream/blob/main/Video.mp4
- added 4 channel support and 4 floating window cams but 1 at a time to ESP project, ill later work on this form maybe add a cool CCTV interface... maybe a button to record to an NVR or SD CARD
- you can control the cameras and floating windows from HA Devices and Service Panel so you dont need to touch ESP screen.

# esp_rtsp_stream

ESPHome external component that shows an RTSP **MJPEG** camera stream (like Hikvision, Dahua and other ONVIF-style IP cameras) on an ESP32-S3 or ESP32-P4 display.

## Prerequisite
- **Dual Core** (currently ESP32-S3 S31 and P4 are equiped with this)
- **PSRAM**
- **MJPEG** stream configured on the camera, normally the sub-stream ( does not decode H.264/H.265 at this stage )

## How it works

1. **RTSP:** RTSP 1.0 runs over one TCP connection, with RTP interleaved on the same socket (`RTP/AVP/TCP`). This avoids lost UDP packets and firewall/NAT problems on WiFi. Basic and Digest authentication are supported (both cameras use Digest by default). An `OPTIONS` keepalive is sent every `timeout/2`.
2. **RTP/JPEG (RFC 2435):** JPEG frames are rebuilt directly in a PSRAM buffer. Supported: types 0/1 (4:2:2 and 4:2:0), types 64/65 (restart markers), Q 1–99 (standard tables) and Q 128–255 (in-band tables).
3. **Decoding:** a separate task decodes each frame to RGB565. A frame that arrives while the decoder is busy, or faster than `max_fps`, is dropped. The picture always shows the newest frame and never builds a backlog.
4. **Display:** decoded frames are double-buffered. `id(cam).draw(it)` draws the newest frame from a display lambda.

## Per-chip profiles

The profile is chosen automatically from the `esp32:` variant/board, so you don't need a "model" option.

| | ESP32-S3 | ESP32-P4 |
|---|---|---|
| Decoder | `espressif/esp_new_jpeg` (software, S3 SIMD) | Hardware JPEG engine |
| Scale / rotate | During decode (down to 1/8, multiples of 8) | PPA (1/16 steps) |
| Exact fit | A final bilinear resize closes the last few pixels | Same |
| Default `max_fps` | 12 | 25 |
| Default `max_frame_size` | 256 KB | 1 MB |
| Recommended stream | 640×480 or 704×576 (D1), MJPEG | Up to 1920×1080 MJPEG, link permitting |
| Expected rate | ~15–20 fps at D1/VGA, if WiFi keeps up | Limited by the network, not the decoder |
| lwIP TCP window | 32 KB | 64 KB |

The network task runs on core 0 and the decoder on core 1. PSRAM is required.

## Camera setup

| Camera | Setting | URL |
|---|---|---|
| Hikvision | Configuration → Video/Audio → *Sub Stream* → Video Encoding **MJPEG** | `rtsp://IP:554/Streaming/Channels/102` |
| Dahua | Setup → Camera → Video → *Sub Stream 1* → Encode Mode **MJPEG** | `rtsp://IP:554/cam/realmonitor?channel=1&subtype=1` |

RTP/JPEG cannot carry frames larger than 2040 px in either direction.

## Configuration

```yaml
external_components:
  - source: github://chickymonkey/Esp-rtsp-stream
    components: [esp_rtsp_stream]

esp_rtsp_stream:
  - id: front_door
    url: rtsp://192.168.1.64:554/Streaming/Channels/102
    username: admin                    # or put user:pass@ in the URL
    password: !secret camera_password
    output_width: 480                  # optional: fit (keeping aspect) into this box
    output_height: 320
    rotation: 0                        # 0/90/180/270, clockwise
    max_fps: 12                        # optional, default from chip profile
    max_frame_size: 262144             # optional, bytes per JPEG frame
    display_id: lcd                    # optional: redraw this display on every new frame
    log_frame_info: true               # fps/decode stats every 30 s
```

Use the `username`/`password` options when the password contains `@`, `/` or `:`.

### ESP32-S3 example

```yaml
esp32:
  variant: esp32s3
  cpu_frequency: 240MHZ
  framework:
    type: esp-idf

psram:
  mode: octal
  speed: 80MHz

display:
  - platform: ...            # your panel
    id: lcd
    update_interval: never   # esp_rtsp_stream triggers updates
    auto_clear_enabled: false
    lambda: |-
      id(front_door).draw(it);          // centred
      // id(front_door).draw(it, 0, 0); // or at a position
```

### ESP32-P4 example

```yaml
esp32:
  variant: esp32p4
  framework:
    type: esp-idf

psram:
  speed: 200MHz

# Network: ethernet (best for high resolution) or esp32_hosted + wifi
```

The display section is the same as for the S3, typically with a `mipi_dsi` panel. For a portrait 800×1280 panel showing a landscape camera, use `rotation: 90` and `output_width: 800` / `output_height: 1280`.



## LVGL

```yaml
esp_rtsp_stream:
  - id: front_door
    url: rtsp://192.168.1.64:554/Streaming/Channels/102
    lvgl_canvas: cam_canvas   # output size defaults to the canvas size

lvgl:
  pages:
    - id: cam_page
      widgets:
        - canvas:
            id: cam_canvas
            x: 0
            y: 0
            width: 320
            height: 240
```
## C++ API

- `draw(it)` / `draw(it, x, y)`: draw the newest frame.
- `has_frame()`, `is_streaming()`, `get_frame_width()`, `get_frame_height()`
- `get_frame_count()`, `get_dropped_frames()`, `get_last_decode_ms()`

## Limitations

- MJPEG only; no H.264/H.265.
- RTP over TCP only. A camera that refuses TCP transport fails at SETUP.
- 8-bit quantisation tables only; 16-bit tables are rare.
- With LVGL, use `lvgl_canvas` (below), not a display lambda: LVGL owns the screen.
The frame is copied into the canvas from the main loop, which is safe for LVGL, and centred on it. While the canvas isn't on screen (another page is showing), frames are not decoded. The RTSP session stays connected, so the picture comes back instantly.

