#pragma once

#include "ocr.hpp"

#include <expected>
#include <filesystem>
#include <map>
#include <string>
#include <string_view>

namespace app {

// Screen regions by name, in pixels of the 1024x768 browser window.
using RegionConfig = std::map<std::string, Box, std::less<>>;

// Name of the region holding the horizontal list of games.
inline constexpr std::string_view kGameListRegion = "GAME_LIST_REGION";

// Parses "[left,top] [width,height]", e.g. "[0,0] [500,100]".
std::expected<Box, std::string> parseRegion(std::string_view value);

// Parses "NAME = [left,top] [width,height]" lines; '#' starts a comment line.
std::expected<RegionConfig, std::string> parseRegionConfig(std::string_view text);

// Reads the region config file.
std::expected<RegionConfig, std::string> loadRegionConfig(const std::filesystem::path& file);

// $THE_APP_AI_REGION_CONFIG if set, otherwise ./region_config.txt
std::filesystem::path regionConfigPath();

// Looks up `name` and checks that the region lies inside a width x height window.
std::expected<Box, std::string> requireRegion(const RegionConfig& config, std::string_view name,
                                              int width, int height);

} // namespace app
