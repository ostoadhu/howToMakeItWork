#pragma once

#include <expected>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace app {

struct Box {
    int left{};
    int top{};
    int width{};
    int height{};

    int centerX() const { return left + width / 2; }
    int centerY() const { return top + height / 2; }
};

// One word recognized by tesseract, with its position in the image (pixels).
struct OcrWord {
    std::string text;
    Box box;
    float confidence{};
};

// Parses tesseract's TSV output (`tesseract img stdout tsv`), keeping word rows only.
// Coordinates are divided by `scale` (use it if the image was enlarged before OCR).
std::vector<OcrWord> parseTesseractTsv(std::string_view tsv, int scale = 1);

// Uppercases and strips Hungarian accents and punctuation: "Összes," -> "OSSZES".
std::string normalizeWord(std::string_view word);

// Finds `phrase` (one or more words) as consecutive words on the same text row.
// Matching is accent/case-insensitive and tolerates small OCR errors.
// Returns the bounding box of the whole phrase.
std::optional<Box> findPhrase(const std::vector<OcrWord>& words, std::string_view phrase);

// Runs tesseract on an image (grayscaled and enlarged first with ImageMagick for
// better accuracy) and returns the words with coordinates of the original image.
std::expected<std::vector<OcrWord>, std::string> recognize(const std::filesystem::path& image,
                                                           const std::string& language = "hun");

} // namespace app
