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

} // namespace app
