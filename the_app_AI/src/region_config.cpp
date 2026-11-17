#include "region_config.hpp"

#include <cstdlib>
#include <fstream>
#include <regex>
#include <sstream>

namespace app {

namespace {

std::string_view trim(std::string_view s)
{
    const auto first = s.find_first_not_of(" \t\r");
    if (first == std::string_view::npos) {
        return {};
    }
    const auto last = s.find_last_not_of(" \t\r");
    return s.substr(first, last - first + 1);
}

std::string describe(const Box& b)
{
    return '[' + std::to_string(b.left) + ',' + std::to_string(b.top) + "] [" + std::to_string(b.width)
        + ',' + std::to_string(b.height) + ']';
}

} // namespace

std::expected<Box, std::string> parseRegion(std::string_view value)
{
    static const std::regex pattern(
        R"(^\s*\[\s*(\d{1,5})\s*,\s*(\d{1,5})\s*\]\s*\[\s*(\d{1,5})\s*,\s*(\d{1,5})\s*\]\s*$)");
    const std::string text(value);
    std::smatch m;
    if (!std::regex_match(text, m, pattern)) {
        return std::unexpected("expected [left,top] [width,height], got '" + text + "'");
    }
    const Box box{std::stoi(m[1]), std::stoi(m[2]), std::stoi(m[3]), std::stoi(m[4])};
    if (box.width <= 0 || box.height <= 0) {
        return std::unexpected("width and height must be positive in " + describe(box));
    }
    return box;
}

std::expected<RegionConfig, std::string> parseRegionConfig(std::string_view text)
{
    RegionConfig config;
    std::size_t start = 0;
    int lineNo = 0;
    while (start <= text.size()) {
        const auto end = text.find('\n', start);
        const auto line = trim(text.substr(start, end == std::string_view::npos ? std::string_view::npos
                                                                                   : end - start));
        ++lineNo;
        start = end == std::string_view::npos ? text.size() + 1 : end + 1;
        if (line.empty() || line.front() == '#') {
            continue;
        }
        const auto eq = line.find('=');
        const auto name = eq == std::string_view::npos ? std::string_view{} : trim(line.substr(0, eq));
        if (name.empty()) {
            return std::unexpected("line " + std::to_string(lineNo) + ": expected NAME = [left,top] [width,height]");
        }
        auto region = parseRegion(line.substr(eq + 1));
        if (!region) {
            return std::unexpected("line " + std::to_string(lineNo) + ": " + region.error());
        }
        if (!config.emplace(std::string(name), *region).second) {
            return std::unexpected("line " + std::to_string(lineNo) + ": " + std::string(name)
                                   + " is defined twice");
        }
    }
    return config;
}

std::expected<RegionConfig, std::string> loadRegionConfig(const std::filesystem::path& file)
{
    std::ifstream in(file);
    if (!in) {
        return std::unexpected("cannot read " + file.string());
    }
    std::stringstream content;
    content << in.rdbuf();
    auto config = parseRegionConfig(content.str());
    if (!config) {
        return std::unexpected(file.string() + ": " + config.error());
    }
    return config;
}

std::filesystem::path regionConfigPath()
{
    if (const char* env = std::getenv("THE_APP_AI_REGION_CONFIG"); env && *env) {
        return env;
    }
    return "region_config.txt";
}

std::expected<Box, std::string> requireRegion(const RegionConfig& config, std::string_view name,
                                              int width, int height)
{
    const auto it = config.find(name);
    if (it == config.end()) {
        return std::unexpected(std::string(name) + " is missing");
    }
    const Box& b = it->second;
    if (b.left + b.width > width || b.top + b.height > height) {
        return std::unexpected(std::string(name) + ' ' + describe(b) + " does not fit in the "
                               + std::to_string(width) + 'x' + std::to_string(height) + " window");
    }
    return b;
}

} // namespace app
