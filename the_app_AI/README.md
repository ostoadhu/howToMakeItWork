# the_app_AI

Minimal C++23 standalone app that drives a browser via external X11 tools
(see `../description.md`).

## Use case 1

`the_app_AI <url> [output.png]`

1. Launches chromium on `<url>` (app mode: window shows only the page content).
2. Finds the window with `xdotool`, activates it.
3. Captures it with ImageMagick `import` (default: `./screenshot.png`).
4. Opens the screenshot in `gimp`.

## Dependencies (Debian)

```sh
sudo apt install build-essential cmake chromium xdotool imagemagick gimp tesseract-ocr
```

GCC 12+ is required (C++23, `std::expected`).

### X11 only (no Wayland)

xdotool and ImageMagick `import` do not work under Wayland. At startup the app
checks `XDG_SESSION_TYPE` / `WAYLAND_DISPLAY` / `DISPLAY` and exits with an error
if a Wayland session (or no display) is detected. Chromium is also forced to
X11 with `--ozone-platform=x11`.

To switch Debian to X11: choose "GNOME on Xorg" at the login screen, or set
`WaylandEnable=false` in `/etc/gdm3/daemon.conf` and reboot.
Check with `echo $XDG_SESSION_TYPE` (must print `x11`).

## Build & run

```sh
cmake -S . -B build
make -C build
./build/the_app_AI https://example.com
```

## Layout

```
CMakeLists.txt
include/   process.hpp  browser.hpp  screenshot.hpp  session.hpp
src/       main.cpp  process.cpp  browser.cpp  screenshot.cpp  session.cpp
```

`process` runs external tools without a shell (no URL injection);
`browser` handles chromium/xdotool; `screenshot` handles import/gimp;
`session` detects X11 vs Wayland.
Tesseract analysis is not part of use case 1 yet.
