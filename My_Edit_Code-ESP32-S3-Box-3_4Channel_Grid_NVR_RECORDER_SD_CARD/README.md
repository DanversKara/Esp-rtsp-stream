# CCTV Box v6 "NVR" — hybrid build

4-camera viewer + SD-card recorder for the ESP32-S3-BOX-3
(+ BOX-3-SENSOR base for the microSD slot). Supersedes v5.

## How this build is put together

| Layer | Source |
|---|---|
| Hardware base (LCD, backlight, battery, speaker/mic, both I2C buses, WiFi + fallback AP, Bluetooth proxy, USB logging) | `Start_From_Scratch_Code.yaml`, kept whole — only its "READY" display lambda is dropped because LVGL owns the screen |
| Stream/scale engine | `esp_rtsp_stream` upstream design: one 320x240 stream per camera, the decoder scales it to the canvas on screen |
| HTTP MJPEG sessions, runtime canvas routing, idle disconnect, AVI recording, SD management | the cctv fork additions (v4 components, byte-identical in this zip) |

## Screens (no floating windows anymore)

- **Viewer = the home page.** One camera full-screen at 320x240. The bottom
  button bar is **hidden** — tap the picture to show it, it hides again
  after 5 s: **1 2 3 4** pick a camera, **FIT/STR/CROP** cycles the picture
  fit, **REC** records the camera you're viewing, **GRID** shows all four.
  Top row: camera name + REC badge (left), clock (centre), WiFi and Home
  Assistant status icons (green = connected) and SD free space (top right).
- **Grid.** All four cameras 2x2 at 160x120 from the same streams (no second
  connection per camera). The **FIT/STR/CROP** button at the top centre sets
  the grid's fit independently of the viewer's. Tap a cell to open that
  camera in the viewer; the red X returns to the viewer.
- The physical red home button under the screen returns to the viewer.

## Picture fit modes (viewer and grid, saved per screen)

| Mode | What it does |
|---|---|
| FIT | Keeps the shape, fits inside the screen, black bars if the shapes differ. Never enlarges. |
| STR | Stretches/squashes to fill the screen exactly. No bars, no cropping, shape may distort. |
| CROP | Fills the screen keeping the shape, centre-cropping whatever overflows. May enlarge a small stream. |

With a 320x240 stream all three look identical in the viewer (1:1) and the
grid (1:2) — the modes matter when a stream's shape differs, e.g. a 16:9
RTSP feed in the 4:3 screen: FIT letterboxes it, CROP fills and trims
the sides, STR squashes it.

## Cameras

| # | Name | Source |
|---|---|---|
| 1 | DOOR | `rtsp://192.168.8.230/live1` — direct from the camera, motionEye not involved |
| 2 | WINDOW | motionEye `http://192.168.8.212:9084` |
| 3 | PARKING LOT | motionEye `http://192.168.8.212:9085` |
| 4 | DOOR 2 | motionEye `http://192.168.8.212:9081` (same picture as cam 1) |

All URLs are the `camN_url` substitutions at the top of `cctv-box.yaml` —
one line each, `rtsp://` and `http://` can be mixed. Cam 2's camera lives at
192.168.8.223; if it serves the same `/live1` path, change `cam2_url` to
`rtsp://192.168.8.223/live1` and it goes direct too.

**RTSP warning:** this box decodes **MJPEG only** (RTP/JPEG). If a camera's
`/live1` is H.264, that camera will sit at "Connecting..." and the log will
show RTSP connecting but no frames ever decoding. Fixes: set that camera's
stream/sub-stream encoding to MJPEG in its own settings, or point the box
back at its motionEye HTTP port (which is MJPEG) — one line in the
substitutions.

## Recording

- REC on the viewer, the Record switches / Record All button in Home
  Assistant. Recordings keep running while a camera is off-screen and
  resume automatically after a reboot.
- Files: `/sdcard/cctv/<YYYYMMDD_HHMMSS>_<NAME>.avi`, Motion-JPEG AVI
  (plays in VLC), 2 fps, split every 5 minutes. Recordings are the raw
  stream frames, not the scaled screen picture.
- When the card drops below 1 GB free, the oldest files are deleted
  (the newest 8 are always kept). Card must be FAT32.

## Deploy

1. Back up your current ESPHome device folder.
2. Unzip this over it (keep your existing `secrets.yaml` — it needs
   `wifi_ssid`, `wifi_password`, `api_key`).
3. `esphome run cctv-box.yaml` (or Upload in the ESPHome dashboard).
4. First boot log lines to expect per camera:
   `First frame: 320x240 stream -> 320x240 output` in the viewer and
   `-> 160x120 output` in the grid.

## Notes

- Bluetooth proxy is kept from the scratch base. It shares the radio with
  WiFi — if the grid stutters, comment out `esp32_ble_tracker:` /
  `bluetooth_proxy:` in the YAML; nothing on the CCTV side uses it.
- Speaker/mic/battery are kept from the scratch base so every piece of
  hardware stays live and testable; the CCTV screens don't use audio.
- Known upstream fault that this build routes around for cam 1 only:
  motionEye still closes HTTP streams after ~10 s, so cams 2–4 can still
  blink/reconnect in the grid until they're switched to direct RTSP too.
