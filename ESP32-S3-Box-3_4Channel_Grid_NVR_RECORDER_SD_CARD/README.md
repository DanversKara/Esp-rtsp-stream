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
