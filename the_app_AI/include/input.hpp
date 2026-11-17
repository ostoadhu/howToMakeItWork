#pragma once

#include <expected>
#include <string>

namespace app {

// Left-clicks at (x, y), relative to the top-left corner of the X11 window,
// i.e. the same coordinate system as a screenshot taken with `import -window`.
std::expected<void, std::string> clickInWindow(const std::string& windowId, int x, int y);

// Types `text` into the focused element of the window, like a user on the keyboard.
std::expected<void, std::string> typeText(const std::string& windowId, const std::string& text);

// Presses one key (xdotool key name, e.g. "Return") in the window.
std::expected<void, std::string> pressKey(const std::string& windowId, const std::string& key);

} // namespace app
