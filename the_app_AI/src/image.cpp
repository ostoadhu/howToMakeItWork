#include "image.hpp"

#include "process.hpp"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cstdlib>

namespace app {

namespace {

// Minimum gray level difference between foreground (border, dot) and background.
constexpr int kContrast = 60;
constexpr int kMinFieldHeight = 12;
constexpr int kMaxFieldHeight = 120;

bool differs(int a, int b)
{
    return std::abs(a - b) > kContrast;
}

bool overlaps(const Box& a, const Box& b)
{
    return a.left < b.left + b.width && b.left < a.left + a.width && a.top < b.top + b.height
        && b.top < a.top + a.height;
}

} // namespace

std::expected<GrayImage, std::string> parsePgm(std::string_view data)
{
    std::size_t pos = 0;
    auto nextToken = [&]() -> std::string_view {
        for (;;) {
            while (pos < data.size() && std::isspace(static_cast<unsigned char>(data[pos]))) {
                ++pos;
            }
            if (pos < data.size() && data[pos] == '#') {
                while (pos < data.size() && data[pos] != '\n') {
                    ++pos;
                }
                continue;
            }
            break;
        }
        const std::size_t start = pos;
        while (pos < data.size() && !std::isspace(static_cast<unsigned char>(data[pos]))) {
            ++pos;
        }
        return data.substr(start, pos - start);
    };
    auto number = [&](int& out) {
        const auto t = nextToken();
        const auto [p, ec] = std::from_chars(t.data(), t.data() + t.size(), out);
        return ec == std::errc{} && p == t.data() + t.size() && !t.empty();
    };

    if (nextToken() != "P5") {
        return std::unexpected("not a binary PGM (P5) image");
    }
    GrayImage img;
    int maxval = 0;
    if (!number(img.width) || !number(img.height) || !number(maxval) || img.width <= 0
        || img.height <= 0 || maxval <= 0 || maxval > 255) {
        return std::unexpected("invalid PGM header");
    }
    ++pos; // exactly one whitespace character separates the header from the pixels
    const auto size = static_cast<std::size_t>(img.width) * static_cast<std::size_t>(img.height);
    if (pos > data.size() || data.size() - pos < size) {
        return std::unexpected("PGM pixel data is truncated");
    }
    img.pixels.assign(reinterpret_cast<const std::uint8_t*>(data.data() + pos),
                      reinterpret_cast<const std::uint8_t*>(data.data() + pos + size));
    return img;
}

std::expected<GrayImage, std::string> loadGray(const std::filesystem::path& file)
{
    auto res = run({"convert", file.string(), "-colorspace", "Gray", "-depth", "8", "pgm:-"});
    if (!res) {
        return std::unexpected(res.error());
    }
    if (res->exitCode != 0) {
        return std::unexpected("convert failed with exit code " + std::to_string(res->exitCode));
    }
    return parsePgm(res->output);
}

std::optional<Box> findInputBelow(const GrayImage& img, const Box& label, int maxGap)
{
    const int x = std::clamp(label.centerX(), 0, img.width - 1);
    int y = label.top + label.height;
    if (y >= img.height) {
        return std::nullopt;
    }
    // Page background, taken just left of the label; skip label descenders below it.
    const int refX = std::clamp(label.left - 4, 0, img.width - 1);
    const int background = img.at(refX, std::clamp(label.centerY(), 0, img.height - 1));
    for (int skip = 0; skip < 8 && y < img.height && differs(img.at(x, y), background); ++skip) {
        ++y;
    }
    if (y >= img.height) {
        return std::nullopt;
    }

    // Going down: background, top border, inside of the field, bottom border.
    const int searchEnd = std::min(img.height, y + maxGap);
    while (y < searchEnd && !differs(img.at(x, y), background)) {
        ++y;
    }
    if (y >= searchEnd) {
        return std::nullopt;
    }
    while (y < img.height && differs(img.at(x, y), background)) {
        ++y;
    }
    const int top = y;
    if (top >= img.height) {
        return std::nullopt;
    }
    const int inside = img.at(x, top);
    const int bottomEnd = std::min(img.height, top + kMaxFieldHeight);
    // A border row differs over a whole horizontal span; text inside the field does not.
    auto isBorderRow = [&](int row) {
        int count = 0;
        int total = 0;
        for (int cx = std::max(0, x - 10); cx <= std::min(img.width - 1, x + 10); ++cx, ++total) {
            count += differs(img.at(cx, row), inside) ? 1 : 0;
        }
        return count * 10 >= total * 9;
    };
    while (y < bottomEnd && !isBorderRow(y)) {
        ++y;
    }
    if (y >= bottomEnd || y - top < kMinFieldHeight) {
        return std::nullopt;
    }
    const int height = y - top;

    // Going left and right from the label column, just below the top border (above any text).
    const int scanY = top + std::max(2, height / 8);
    int left = x;
    while (left > 0 && !differs(img.at(left - 1, scanY), inside)) {
        --left;
    }
    int right = x;
    while (right < img.width - 1 && !differs(img.at(right + 1, scanY), inside)) {
        ++right;
    }
    if (left == 0 || right == img.width - 1 || right - left + 1 < 3 * height) {
        return std::nullopt; // no side borders, or not shaped like a text field
    }
    return Box{left, top, right - left + 1, height};
}

std::vector<Box> findDots(const GrayImage& img, const Box& region)
{
    const int x0 = std::max(0, region.left);
    const int y0 = std::max(0, region.top);
    const int x1 = std::min(img.width, region.left + region.width);
    const int y1 = std::min(img.height, region.top + region.height);
    if (x0 >= x1 || y0 >= y1) {
        return {};
    }
    const int w = x1 - x0;
    const int h = y1 - y0;

    std::vector<std::uint8_t> values;
    values.reserve(static_cast<std::size_t>(w) * h);
    for (int y = y0; y < y1; ++y) {
        for (int x = x0; x < x1; ++x) {
            values.push_back(img.at(x, y));
        }
    }
    auto sorted = values;
    std::nth_element(sorted.begin(), sorted.begin() + sorted.size() / 2, sorted.end());
    const int background = sorted[sorted.size() / 2];

    std::vector<std::uint8_t> seen(values.size(), 0);
    std::vector<Box> dots;
    std::vector<int> stack;
    for (int start = 0; start < w * h; ++start) {
        if (seen[start] || !differs(values[start], background)) {
            continue;
        }
        // Flood fill one 8-connected component.
        int minX = w, minY = h, maxX = -1, maxY = -1, area = 0;
        stack.assign(1, start);
        seen[start] = 1;
        while (!stack.empty()) {
            const int i = stack.back();
            stack.pop_back();
            const int cx = i % w;
            const int cy = i / w;
            ++area;
            minX = std::min(minX, cx);
            maxX = std::max(maxX, cx);
            minY = std::min(minY, cy);
            maxY = std::max(maxY, cy);
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dx = -1; dx <= 1; ++dx) {
                    const int nx = cx + dx;
                    const int ny = cy + dy;
                    if (nx < 0 || ny < 0 || nx >= w || ny >= h) {
                        continue;
                    }
                    const int n = ny * w + nx;
                    if (!seen[n] && differs(values[n], background)) {
                        seen[n] = 1;
                        stack.push_back(n);
                    }
                }
            }
        }
        const int bw = maxX - minX + 1;
        const int bh = maxY - minY + 1;
        const bool touchesEdge = minX == 0 || minY == 0 || maxX == w - 1 || maxY == h - 1;
        const bool dotSized = bw >= 3 && bh >= 3 && bw <= h && bh <= h;
        const bool round = std::abs(bw - bh) <= std::max(1, std::max(bw, bh) / 3);
        const bool filled = area * 100 >= bw * bh * 55;
        if (!touchesEdge && dotSized && round && filled) {
            dots.push_back({x0 + minX, y0 + minY, bw, bh});
        }
    }
    return dots;
}

int countNewDots(const std::vector<Box>& before, const std::vector<Box>& after)
{
    std::vector<Box> fresh;
    for (const auto& a : after) {
        if (std::none_of(before.begin(), before.end(), [&](const Box& b) { return overlaps(a, b); })) {
            fresh.push_back(a);
        }
    }
    int best = 0;
    for (const auto& c : fresh) {
        const int count = static_cast<int>(std::count_if(fresh.begin(), fresh.end(), [&](const Box& d) {
            return std::abs(d.centerY() - c.centerY()) <= std::max(2, c.height / 2)
                && std::abs(d.width - c.width) <= 2 && std::abs(d.height - c.height) <= 2;
        }));
        best = std::max(best, count);
    }
    return best;
}

} // namespace app
