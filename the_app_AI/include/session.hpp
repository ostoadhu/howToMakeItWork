#pragma once

#include <expected>
#include <string>

namespace app {

enum class SessionType { X11, Wayland, Unknown };

// Detects the graphical session type from the environment
// (XDG_SESSION_TYPE, WAYLAND_DISPLAY, DISPLAY).
SessionType detectSession();

std::string toString(SessionType type);

// xdotool and ImageMagick `import` only work on X11. Fails with an explanatory
// message if the session is Wayland or no X11 display is available.
std::expected<void, std::string> requireX11Session();

} // namespace app
