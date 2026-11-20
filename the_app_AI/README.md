# the_app_AI

Minimal C++23 standalone app that drives a browser via external X11 tools
(see `../description.md`).

## Usage

`the_app_AI [url] [output.png]`

`url` defaults to `https://www.tippmixpro.hu`, `output.png` to `./screenshot.png`.

What the app does is described by the use cases in
[`docs/use_cases.md`](docs/use_cases.md) (UseCase_1: open page, screenshot,
GIMP; UseCase_2: reject the cookie banner; UseCase_3: open the window at
1600x768; UseCase_4: close the browser window; UseCase_5: find and click the
"Bejelentkezés" login button; UseCase_6: select the game type; UseCase_7:
enter and validate the login e-mail address and password; UseCase_8: read the
list of games; UseCase_9: open the page in incognito mode).

### Screen regions

Screen regions are configured in `region_config.txt` (working directory, or
the file named by `THE_APP_AI_REGION_CONFIG`), in pixels of the 1600x768 window:

```
GAME_LIST_REGION = [0,0] [500,100]   # [left,top] [width,height]
```

### Login credentials

The e-mail address and password are read from `credentials.env` in the
working directory (or the file named by `THE_APP_AI_CREDENTIALS`). The file is
gitignored; create it from the template:

```sh
cp credentials.env.example credentials.env   # then edit EMAIL= and PASSWORD=
chmod 600 credentials.env
```

The password is never logged, only its length and the number of dots seen.

### Monitoring log

Every step is logged with a timestamp and the state machine state, both to
the terminal and appended to `the_app_AI.log` in the working directory:

```
2026-11-16 12:44:02.123 [INFO] [Login] login button "Bejelentkezés" found: click point 960,32, box 100x16 at 910,24
2026-11-16 12:44:02.311 [INFO] [Login] login button clicked
```

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

## State machine diagram

States and transitions live in `docs/state_machine.yaml`; the Mermaid diagram
in `docs/use_cases.md` is generated from it (GitHub renders it as a picture).
After changing a use case/state, update the YAML and regenerate:

```sh
sudo apt install python3-yaml        # or: pip install pyyaml
python3 tools/gen_state_diagram.py   # --check only verifies
```

Enable the pre-commit hook once per clone (run in the repo root); it
regenerates the diagram on every commit touching these files:

```sh
git config core.hooksPath .githooks
```

The CI workflow `.github/workflows/state-diagram.yml` fails if the committed
diagram is out of date.

## Layout

```
CMakeLists.txt  credentials.env.example  region_config.txt
docs/      use_cases.md  state_machine.yaml  login_popup.png
include/   process.hpp  browser.hpp  screenshot.hpp  session.hpp  ocr.hpp  input.hpp  log.hpp  image.hpp  credentials.hpp  region_config.hpp
src/       main.cpp  process.cpp  browser.cpp  screenshot.cpp  session.cpp  ocr.cpp  input.cpp  log.cpp  image.cpp  credentials.cpp  region_config.cpp
tests/     CMakeLists.txt  test_*.cpp
tools/     gen_state_diagram.py
```

`process` runs external tools without a shell (no URL injection);
`browser` handles chromium/xdotool; `screenshot` handles import/gimp;
`session` detects X11 vs Wayland; `ocr` runs tesseract and locates text
on the screenshot; `input` clicks and types inside the window with xdotool;
`image` finds input fields and password dots in the screenshot pixels;
`credentials` reads `credentials.env`; `region_config` reads `region_config.txt`;
`log` writes the timestamped
monitoring log.
