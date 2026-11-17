#pragma once

#include <chrono>
#include <expected>
#include <optional>
#include <string>
#include <sys/types.h>
#include <vector>

namespace app {

struct WindowSize {
    int width;
    int height;

    bool operator==(const WindowSize&) const = default;
};

std::string toString(WindowSize size);

// Parses the output of `xdotool getwindowgeometry --shell` (WIDTH=... / HEIGHT=... lines).
std::optional<WindowSize> parseWindowGeometry(const std::string& shellOutput);

// Returns the current size of an X11 window.
std::expected<WindowSize, std::string> windowSize(const std::string& windowId);

// Resizes the window until it has exactly `size`, up to `attempts` tries.
std::expected<void, std::string> ensureWindowSize(const std::string& windowId, WindowSize size,
                                                  int attempts = 3);

// Command line used to launch chromium: a private window (--incognito, so no cookies,
// history or cache are kept) in a dedicated profile directory, at `size`, showing `url`.
std::vector<std::string> chromiumCommand(const std::string& url, WindowSize size,
                                         const std::string& profileDir);

// Launches chromium in an incognito window (see chromiumCommand).
class Browser {
public:
    std::expected<void, std::string> open(const std::string& url, WindowSize size);

    // Waits until the browser window is visible and returns its X11 window id.
    std::expected<std::string, std::string> waitForWindow(std::chrono::seconds timeout) const;

    // Pid of the launched chromium process, -1 if not started.
    pid_t pid() const { return pid_; }

private:
    pid_t pid_{-1};
};

} // namespace app
