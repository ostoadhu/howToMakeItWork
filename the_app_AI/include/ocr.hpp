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

// Keeps only the words whose box lies completely inside `region`.
std::vector<OcrWord> wordsInRegion(const std::vector<OcrWord>& words, const Box& region);

// True if `ocrText` (any punctuation/spacing) matches `expected` accent/case-insensitively,
// tolerating ~1 OCR error per 4 characters. Example: "ostorhazyadam@gmai1.com".
bool fuzzyMatches(std::string_view ocrText, std::string_view expected);

// The upper right corner of a width x height image: its right half and top 20%.
Box upperRightCorner(int width, int height);

// Runs tesseract on an image (grayscaled and enlarged first with ImageMagick for
// better accuracy) and returns the words with coordinates of the original image.
std::expected<std::vector<OcrWord>, std::string> recognize(const std::filesystem::path& image,
                                                           const std::string& language = "hun");

// Like recognize(), but only reads `region` of the image (it is cropped first, so the
// rest of the page cannot disturb the layout analysis). Coordinates are of the whole image.
std::expected<std::vector<OcrWord>, std::string> recognizeRegion(const std::filesystem::path& image,
                                                                 const Box& region,
                                                                 const std::string& language = "hun");

// One entry of a horizontal list (menu, tabs): adjacent words joined with spaces.
struct ListItem {
    std::string text;
    Box box;
};

// Groups words into list items: rows top to bottom, items left to right. Words on the
// same row belong to one item if the gap between them is at most `maxGapFactor` times
// the text height (a word space); a wider gap starts a new item. Punctuation-only
// words (icons read as "|", "•", ...) are dropped.
std::vector<ListItem> groupListItems(std::vector<OcrWord> words, double maxGapFactor = 1.0);

} // namespace app
