#include "input.hpp"

#include "process.hpp"

namespace app {

std::expected<void, std::string> clickInWindow(const std::string& windowId, int x, int y)
{
    (void)run({"xdotool", "windowactivate", "--sync", windowId});

    auto res = run({"xdotool", "mousemove", "--window", windowId, std::to_string(x),
                    std::to_string(y), "click", "1"});
    if (!res) {
        return std::unexpected(res.error());
    }
    if (res->exitCode != 0) {
        return std::unexpected("xdotool click failed with exit code "
                               + std::to_string(res->exitCode));
    }
    return {};
}

std::expected<void, std::string> typeText(const std::string& windowId, const std::string& text)
{
    (void)run({"xdotool", "windowactivate", "--sync", windowId});

    // "--" so that text starting with '-' is not taken as an option.
    auto res = run({"xdotool", "type", "--clearmodifiers", "--delay", "60", "--", text});
    if (!res) {
        return std::unexpected(res.error());
    }
    if (res->exitCode != 0) {
        return std::unexpected("xdotool type failed with exit code "
                               + std::to_string(res->exitCode));
    }
    return {};
}

std::expected<void, std::string> pressKey(const std::string& windowId, const std::string& key)
{
    (void)run({"xdotool", "windowactivate", "--sync", windowId});

    auto res = run({"xdotool", "key", "--clearmodifiers", key});
    if (!res) {
        return std::unexpected(res.error());
    }
    if (res->exitCode != 0) {
        return std::unexpected("xdotool key " + key + " failed with exit code "
                               + std::to_string(res->exitCode));
    }
    return {};
}

} // namespace app
