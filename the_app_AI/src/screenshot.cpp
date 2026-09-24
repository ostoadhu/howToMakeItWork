#include "screenshot.hpp"

#include "process.hpp"

namespace app {

std::expected<void, std::string> captureWindow(const std::string& windowId,
                                               const std::filesystem::path& file)
{
    // Bring the window to front so no other window overlaps the capture.
    (void)run({"xdotool", "windowactivate", "--sync", windowId});

    auto res = run({"import", "-window", windowId, file.string()});
    if (!res) {
        return std::unexpected(res.error());
    }
    if (res->exitCode != 0) {
        return std::unexpected("import failed with exit code " + std::to_string(res->exitCode));
    }
    return {};
}

std::expected<void, std::string> openInGimp(const std::filesystem::path& file)
{
    auto pid = spawnDetached({"gimp", file.string()});
    if (!pid) {
        return std::unexpected(pid.error());
    }
    return {};
}

} // namespace app
