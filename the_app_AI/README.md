# the_app_AI

Minimal C++23 standalone app that drives a browser via external X11 tools
(see `../description.md`).

## Usage

`the_app_AI [url] [output.png]`

`url` defaults to `https://www.tippmixpro.hu`, `output.png` to `./screenshot.png`.

What the app does is described by the use cases in
[`docs/use_cases.md`](docs/use_cases.md) (UseCase_1: open page, screenshot,
GIMP; UseCase_2: reject the cookie banner; UseCase_3: open the window at
1024x768; UseCase_4: close the browser window).

### How the screenshot is "read"

- The image is grayscaled and enlarged 2x (`convert`) for better OCR accuracy.
- `tesseract <img> stdout -l hun --psm 11 tsv` returns every word with its
  pixel box; coordinates are scaled back to the original screenshot.
- Words are compared case- and accent-insensitively ("Í" = "I") with a small
  edit-distance tolerance, since OCR often misreads a letter (e.g. `ELUTASlTÁSA`).
- The words of the label must be adjacent on the same row; their combined box
  gives the click point. `import -window` and `xdotool mousemove --window` use
  the same window-relative coordinates, so no conversion is needed.

## Dependencies (Debian)

```sh
sudo apt install build-essential cmake chromium xdotool imagemagick gimp tesseract-ocr tesseract-ocr-hun
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
./build/the_app_AI                       # opens https://www.tippmixpro.hu
./build/the_app_AI https://example.com   # opens a custom url
```

## Tests

Unit tests use GoogleTest (downloaded automatically by CMake `FetchContent`).
Every `tests/test_*.cpp` file is picked up automatically.

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Disable them with `-DBUILD_TESTING=OFF`.

## Layout

```
CMakeLists.txt
include/   process.hpp  browser.hpp  screenshot.hpp  session.hpp  ocr.hpp  input.hpp
src/       main.cpp  process.cpp  browser.cpp  screenshot.cpp  session.cpp  ocr.cpp  input.cpp
tests/     CMakeLists.txt  test_*.cpp
```

`process` runs external tools without a shell (no URL injection);
`browser` handles chromium/xdotool; `screenshot` handles import/gimp;
`session` detects X11 vs Wayland; `ocr` runs tesseract and locates text
on the screenshot; `input` clicks inside the window with xdotool.
