#pragma once

#include <chrono>
#include <expected>
#include <string>
#include <sys/types.h>

namespace app {

// Launches chromium in "app" mode (no tabs/address bar), so the window
// contains only the web page content.
class Browser {
public:
    std::expected<void, std::string> open(const std::string& url);

    // Waits until the browser window is visible and returns its X11 window id.
    std::expected<std::string, std::string> waitForWindow(std::chrono::seconds timeout) const;

private:
    pid_t pid_{-1};
};

} // namespace app
