#pragma once

#include "ocr.hpp"

#include <cstdint>
#include <expected>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace app {

// 8-bit grayscale image, row-major.
struct GrayImage {
    int width{};
    int height{};
    std::vector<std::uint8_t> pixels;

    std::uint8_t at(int x, int y) const { return pixels[static_cast<std::size_t>(y) * width + x]; }
};

// Parses a binary PGM (P5, maxval <= 255), as written by `convert ... pgm:-`.
std::expected<GrayImage, std::string> parsePgm(std::string_view data);

// Loads an image file as grayscale (via ImageMagick `convert`).
std::expected<GrayImage, std::string> loadGray(const std::filesystem::path& file);

// Finds an outlined input field directly below a text label (label above the field,
// as in the tippmixpro login popup). Returns the inside of the field.
std::optional<Box> findInputBelow(const GrayImage& image, const Box& label, int maxGap = 80);

// Finds small filled round blobs (e.g. password dots) inside `region`.
std::vector<Box> findDots(const GrayImage& image, const Box& region);

// Counts the blobs of `after` that were not in `before` and form one row of
// equally sized dots (what a password field shows after typing).
int countNewDots(const std::vector<Box>& before, const std::vector<Box>& after);

} // namespace app
