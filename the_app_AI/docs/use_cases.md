# Use Cases

Use cases for `the_app_AI`, each with the state machine designed to meet it.

## Entry template

```
## UC-NN: <short title>
**Status:** captured | designed | implemented
**Description:** <the use case as described>
**Preconditions:** <what must be true before it starts>
**Steps / expected behaviour:** <numbered list>
**Success:** <end condition>
**Failure / edge cases:** <timeouts, missing elements, popups, ...>
**State machine:** <diagram + states/events/guards/actions table, or link to it>
**Notes / open questions:**
```

---

## UC-01: Reject cookie banner
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
