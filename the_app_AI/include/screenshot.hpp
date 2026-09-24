#pragma once

#include <expected>
#include <filesystem>
#include <string>

namespace app {

// Captures the given X11 window into an image file using ImageMagick's `import`.
std::expected<void, std::string> captureWindow(const std::string& windowId,
                                               const std::filesystem::path& file);

// Opens an image with gimp in the background.
std::expected<void, std::string> openInGimp(const std::filesystem::path& file);

} // namespace app
