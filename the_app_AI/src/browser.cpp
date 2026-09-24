#include "browser.hpp"

#include "process.hpp"

#include <thread>
#include <unistd.h>

namespace app {

std::expected<void, std::string> Browser::open(const std::string& url)
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
        "--start-maximized",
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
