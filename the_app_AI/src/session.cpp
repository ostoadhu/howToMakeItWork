#include "session.hpp"

#include <cstdlib>
#include <string_view>

namespace app {

namespace {

std::string_view env(const char* name)
{
    const char* value = std::getenv(name);
    return value ? std::string_view(value) : std::string_view();
}

} // namespace

SessionType detectSession()
{
    const auto type = env("XDG_SESSION_TYPE");
    if (type == "wayland" || !env("WAYLAND_DISPLAY").empty()) {
        return SessionType::Wayland;
    }
    if (type == "x11" || !env("DISPLAY").empty()) {
        return SessionType::X11;
    }
    return SessionType::Unknown;
}

std::string toString(SessionType type)
{
    switch (type) {
    case SessionType::X11:
        return "x11";
    case SessionType::Wayland:
        return "wayland";
    case SessionType::Unknown:
        break;
    }
    return "unknown";
}

std::expected<void, std::string> requireX11Session()
{
    switch (detectSession()) {
    case SessionType::X11:
        return {};
    case SessionType::Wayland:
        return std::unexpected(
            "Wayland session detected: xdotool and ImageMagick 'import' require X11. "
            "Log out and select an Xorg session (e.g. 'GNOME on Xorg') at the login screen, "
            "or set WaylandEnable=false in /etc/gdm3/daemon.conf");
    case SessionType::Unknown:
        break;
    }
    return std::unexpected("no graphical session detected (DISPLAY is not set); "
                           "an X11 session is required");
}

} // namespace app
