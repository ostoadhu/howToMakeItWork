# Use Cases

Use cases for `the_app_AI`, each with the state machine designed to meet it.
This file is the single source of truth for use cases; general requirements
live in `../../description.md`.

> **Note (future architecture):** the app will move to a state model
> architecture in which each use case becomes a state. Keep each use case
> self-contained, with clear entry conditions (preconditions), an exit
> condition (success) and failure outcomes, so it can later map directly
> to a state and its transitions.
>
> Planned cycle: the first state opens the browser window (UseCase_1,
> UseCase_3), the following states do the work (e.g. take a bet), and the
> last state always closes the window (UseCase_4) before returning to the
> first state. Reloading the page = closing and reopening the browser.

## State machine

The diagram below is **generated**; do not edit it by hand. States and
transitions are defined in [`state_machine.yaml`](state_machine.yaml).
Whenever a use case/state is added or modified:

1. update `state_machine.yaml` (every `UseCase_N` must be covered by a state),
2. run `python the_app_AI/tools/gen_state_diagram.py` (needs PyYAML).

The git pre-commit hook regenerates it automatically, and the CI check
(`.github/workflows/state-diagram.yml`) fails if it is out of date.
GitHub renders the Mermaid block as a picture.

<!-- BEGIN GENERATED STATE MACHINE -->
<!-- Generated from docs/state_machine.yaml by tools/gen_state_diagram.py. Do not edit by hand. -->

```mermaid
stateDiagram-v2
    [*] --> OpenWindow
    state "OpenWindow (UseCase_1, UseCase_3)" as OpenWindow
    OpenWindow : launch Chromium at 1024x768, find the window
    state "WaitPageLoad (UseCase_1)" as WaitPageLoad
    WaitPageLoad : wait for the page, capture the window
    state BannerVisible <<choice>>
    state "RejectCookies (UseCase_2)" as RejectCookies
    RejectCookies : click ÖSSZES ELUTASÍTÁSA, verify banner is gone
    state "TakeBet - not defined yet" as TakeBet
    TakeBet : SelectButtonA / take a bet
    state "CloseWindow (UseCase_4)" as CloseWindow
    CloseWindow : close Chromium, remove temporary profile
    state "Error" as Error
    Error : no handling defined yet, the app exits
    OpenWindow --> WaitPageLoad : window found, size 1024x768
    OpenWindow --> Error : launch fails / no window / size wrong
    WaitPageLoad --> BannerVisible : page loaded, screenshot taken
    BannerVisible --> RejectCookies : banner visible
    BannerVisible --> TakeBet : no banner
    RejectCookies --> TakeBet : banner gone
    RejectCookies --> Error : still visible after retries / OCR fails
    TakeBet --> CloseWindow : bet taken
    CloseWindow --> OpenWindow : window closed, profile removed
    Error --> [*] : exit
```

| State | Use cases | Description |
|---|---|---|
| OpenWindow | UseCase_1, UseCase_3 | launch Chromium at 1024x768, find the window |
| WaitPageLoad | UseCase_1 | wait for the page, capture the window |
| BannerVisible | – | decision |
| RejectCookies | UseCase_2 | click ÖSSZES ELUTASÍTÁSA, verify banner is gone |
| TakeBet | – | SelectButtonA / take a bet *(not defined yet)* |
| CloseWindow | UseCase_4 | close Chromium, remove temporary profile |
| Error | – | no handling defined yet, the app exits |

| From | Event / guard | To |
|---|---|---|
| OpenWindow | window found, size 1024x768 | WaitPageLoad |
| OpenWindow | launch fails / no window / size wrong | Error |
| WaitPageLoad | page loaded, screenshot taken | BannerVisible |
| BannerVisible | banner visible | RejectCookies |
| BannerVisible | no banner | TakeBet |
| RejectCookies | banner gone | TakeBet |
| RejectCookies | still visible after retries / OCR fails | Error |
| TakeBet | bet taken | CloseWindow |
| CloseWindow | window closed, profile removed | OpenWindow |
| Error | exit | [*] |

<!-- END GENERATED STATE MACHINE -->

## Entry template

```
## UseCase_N: <short title>
**Status:** captured | designed | implemented
**Description:** <the use case as described>
**Preconditions:** <what must be true before it starts>
**Steps / expected behaviour:** <numbered list>
**Success:** <end condition>
**Failure / edge cases:** <timeouts, missing elements, popups, ...>
**State machine:** <state(s) covering it in state_machine.yaml; see "State machine" above>
**Notes / open questions:**
```

---

## UseCase_1: Open a webpage, screenshot it, show it in GIMP
**Status:** implemented
**Description:** Start the app from a terminal with a webpage URL as the argument. The app launches Chromium on that page, takes a screenshot of the page content in the browser window, and opens the screenshot in GIMP.
**Preconditions:** X11 session (not Wayland); chromium, xdotool, imagemagick and gimp installed.
**Steps / expected behaviour:**
1. `the_app_AI [url] [output.png]`. `url` defaults to `https://www.tippmixpro.hu`, `output.png` to `./screenshot.png`.
2. Launch Chromium on `url` in app mode, so the window shows only the page content.
3. Find the window with `xdotool` and activate it.
4. Capture the window with ImageMagick `import`.
5. Open the screenshot in `gimp`.
**Success:** The screenshot of the page is open in GIMP.
**Failure / edge cases:** Wayland session or no display detected (exit with an error). Chromium window not found. Screenshot capture fails.
**State machine:** not designed yet.
**Notes / open questions:** UseCase_2 runs between steps 4 and 5.

## UseCase_2: Reject cookie banner
**Status:** captured
**Description:** When the page opens, click "ÖSSZES ELUTASÍTÁSA" on the cookie banner. Continue only if that succeeds.
**Preconditions:** Chromium window found, page loaded.
**Steps / expected behaviour:**
1. Capture the window and run OCR on it.
2. If the banner is visible, click the reject button.
3. Re-capture and check that the banner is gone.
**Success:** The banner is not visible, or was never shown.
**Failure / edge cases:** The banner is still visible after the retries are used up. OCR fails.
**State machine:** not designed yet. The draft flow from the lecture covers it (`WaitPageLoad → ◇ → RejectCookies → SelectButtonA`).
**Notes / open questions:** The user said "összes megtagadása", but the code uses "ÖSSZES ELUTASÍTÁSA". Check which label the site shows.
A first version is already in the code (see "How the screenshot is read" in `../README.md`). There an OCR failure is only a warning and the run continues, which does not match "continue only if that succeeds" above.

## UseCase_3: Open the browser window at a fixed size
**Status:** implemented
**Description:** When the browser window is opened, it must always open at a predefined size: 1024x768.
**Preconditions:** X11 session; Chromium is being launched by UseCase_1.
**Steps / expected behaviour:**
1. Launch Chromium with `--window-size=1024,768` and `--window-position=0,0`.
2. After the window is found, read its size with `xdotool getwindowgeometry --shell`.
3. If the size differs, set it with `xdotool windowsize` and check again (up to 3 tries).
**Success:** The window is 1024x768 before the page-load wait and the first screenshot, so every screenshot is 1024x768.
**Failure / edge cases:** The window manager overrides the size or keeps the window maximized. If the size is still wrong after the tries, the app exits with an error.
**State machine:** not designed yet.
**Notes / open questions:** The size is the outer Chromium window, meaning the X11 window that xdotool reports and ImageMagick `import` captures. Window-manager decorations around it are not counted. The size is defined by `kWindowSize` in `src/main.cpp`.
Test: `tests/test_window_size.cpp` (`BrowserWindow.OpensAt1024x768AccordingToXdotool`) launches Chromium and checks WIDTH/HEIGHT from `xdotool getwindowgeometry --shell`. It is skipped without an X11 session, chromium or xdotool.

## UseCase_4: Close the browser window
**Status:** captured
**Description:** Reloading the page is done by restarting the browser: after every bet taken, the browser window is closed and then reopened, starting again from the beginning. This use case closes the browser window. In the state model it is always the last state of a cycle, right before returning to the first state, where the window is opened (UseCase_1 / UseCase_3).
**Preconditions:** The browser was launched by the app (its pid and window id are known). The work of the cycle is done (e.g. a bet was taken).
**Steps / expected behaviour:**
1. Close the Chromium window gracefully (e.g. `xdotool windowclose` or SIGTERM to the Chromium process group).
2. Wait until the window is gone (`xdotool search --pid` finds nothing) and the process has exited.
3. If it is still running after a timeout, kill it (SIGKILL).
4. Remove the temporary Chromium profile (`/tmp/the_app_AI_profile_<pid>`).
5. Hand over to the first state: open the window again.
**Success:** No window or process of the launched Chromium remains, the temporary profile is removed, and the next cycle can start from a clean state.
**Failure / edge cases:** The window does not close (hung page, dialog such as "Leave site?"). The process does not exit even after SIGKILL. The window was already closed (e.g. crashed): treat as success.
**State machine:** not designed yet. Cycle: `OpenWindow → ... → TakeBet → CloseWindow → OpenWindow`.
**Notes / open questions:** The "take a bet" use case does not exist yet. Should failures in other states also go through CloseWindow before restarting? Should the cycle repeat forever, or stop after a number of bets? Today the app leaves Chromium running when it exits.
