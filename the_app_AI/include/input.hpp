#pragma once

#include <expected>
#include <string>

namespace app {

// Left-clicks at (x, y), relative to the top-left corner of the X11 window,
// i.e. the same coordinate system as a screenshot taken with `import -window`.
std::expected<void, std::string> clickInWindow(const std::string& windowId, int x, int y);

} // namespace app
