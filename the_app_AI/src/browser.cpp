#include "browser.hpp"

#include "process.hpp"

#include <charconv>
#include <sstream>
#include <thread>
#include <unistd.h>

namespace app {

std::string toString(WindowSize size)
{
    return std::to_string(size.width) + 'x' + std::to_string(size.height);
}

std::optional<WindowSize> parseWindowGeometry(const std::string& shellOutput)
{
    std::optional<int> width;
    std::optional<int> height;

    std::istringstream lines(shellOutput);
    std::string line;
    while (std::getline(lines, line)) {
        const auto eq = line.find('=');
        if (eq == std::string::npos) {
            continue;
        }
        const auto key = line.substr(0, eq);
        if (key != "WIDTH" && key != "HEIGHT") {
            continue;
        }
        const char* first = line.data() + eq + 1;
        const char* last = line.data() + line.size();
        int value = 0;
        const auto [ptr, ec] = std::from_chars(first, last, value);
        if (ec != std::errc{} || ptr != last || value <= 0) {
            return std::nullopt;
        }
        (key == "WIDTH" ? width : height) = value;
    }
    if (!width || !height) {
        return std::nullopt;
    }
    return WindowSize{*width, *height};
}

std::expected<WindowSize, std::string> windowSize(const std::string& windowId)
{
    auto res = run({"xdotool", "getwindowgeometry", "--shell", windowId});
    if (!res) {
        return std::unexpected(res.error());
    }
    if (res->exitCode != 0) {
        return std::unexpected("xdotool getwindowgeometry failed with exit code "
                               + std::to_string(res->exitCode));
    }
    auto size = parseWindowGeometry(res->output);
    if (!size) {
        return std::unexpected("cannot parse xdotool getwindowgeometry output");
    }
    return *size;
}

std::expected<void, std::string> ensureWindowSize(const std::string& windowId, WindowSize size,
                                                  int attempts)
{
    auto current = windowSize(windowId);
    for (int i = 0; i < attempts; ++i) {
        if (!current) {
            return std::unexpected(current.error());
        }
        if (*current == size) {
            return {};
        }
        auto res = run({"xdotool", "windowsize", "--sync", windowId, std::to_string(size.width),
                        std::to_string(size.height)});
        if (!res) {
            return std::unexpected(res.error());
        }
        // The window manager applies the new size asynchronously.
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
        current = windowSize(windowId);
    }
    if (!current) {
        return std::unexpected(current.error());
    }
    if (*current == size) {
        return {};
    }
    return std::unexpected("window is " + toString(*current) + " instead of " + toString(size));
}

std::expected<void, std::string> Browser::open(const std::string& url, WindowSize size)
{
    // A dedicated profile forces a new chromium instance, so the window
    // belongs to the pid we launched (instead of an already running chromium).
    const std::string profile = "/tmp/the_app_AI_profile_" + std::to_string(getpid());

    auto pid = spawnDetached({
        "chromium",
        "--user-data-dir=" + profile,
        "--ozone-platform=x11", // never use native Wayland: xdotool must see the window
        "--no-first-run",
        "--no-default-browser-check",
        "--window-position=0,0",
        "--window-size=" + std::to_string(size.width) + ',' + std::to_string(size.height),
        "--app=" + url,
    });
    if (!pid) {
        return std::unexpected(pid.error());
    }
    pid_ = *pid;
    return {};
}

std::expected<std::string, std::string> Browser::waitForWindow(std::chrono::seconds timeout) const
{
    if (pid_ < 0) {
        return std::unexpected("browser not started");
    }

    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline) {
        auto res = run({"xdotool", "search", "--onlyvisible", "--pid", std::to_string(pid_)});
        if (!res) {
            return std::unexpected(res.error());
        }
        if (res->exitCode == 0 && !res->output.empty()) {
            // Take the last listed window: the top-level one created last.
            std::string out = res->output;
            while (!out.empty() && out.back() == '\n') {
                out.pop_back();
            }
            const auto pos = out.find_last_of('\n');
            return pos == std::string::npos ? out : out.substr(pos + 1);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }
    return std::unexpected("timeout waiting for chromium window");
}

} // namespace app
