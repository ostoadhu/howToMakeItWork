#include "ocr.hpp"

#include "process.hpp"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <unistd.h>

namespace app {

namespace {

constexpr int kOcrScale = 2;

std::vector<std::string_view> split(std::string_view s, char sep)
{
    std::vector<std::string_view> parts;
    std::size_t start = 0;
    for (;;) {
        const auto pos = s.find(sep, start);
        parts.push_back(s.substr(start, pos - start));
        if (pos == std::string_view::npos) {
            break;
        }
        start = pos + 1;
    }
    return parts;
}

template <typename T>
bool parseNumber(std::string_view s, T& out)
{
    const auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), out);
    return ec == std::errc{} && ptr == s.data() + s.size();
}

// Decodes one UTF-8 code point starting at s[i] and advances i.
char32_t nextCodePoint(std::string_view s, std::size_t& i)
{
    const auto c = static_cast<unsigned char>(s[i]);
    int len = 1;
    char32_t cp = c;
    if (c >= 0xF0) {
        len = 4;
        cp = c & 0x07;
    } else if (c >= 0xE0) {
        len = 3;
        cp = c & 0x0F;
    } else if (c >= 0xC0) {
        len = 2;
        cp = c & 0x1F;
    }
    for (int k = 1; k < len && i + k < s.size(); ++k) {
        cp = (cp << 6) | (static_cast<unsigned char>(s[i + k]) & 0x3F);
    }
    i += static_cast<std::size_t>(len);
    return cp;
}

char foldHungarian(char32_t cp)
{
    switch (cp) {
    case 0xC1: case 0xE1: return 'A';                         // Á á
    case 0xC9: case 0xE9: return 'E';                         // É é
    case 0xCD: case 0xED: return 'I';                         // Í í
    case 0xD3: case 0xF3: case 0xD6: case 0xF6:               // Ó ó Ö ö
    case 0x150: case 0x151: return 'O';                       // Ő ő
    case 0xDA: case 0xFA: case 0xDC: case 0xFC:               // Ú ú Ü ü
    case 0x170: case 0x171: return 'U';                       // Ű ű
    default: return '\0';
    }
}

std::size_t editDistance(std::string_view a, std::string_view b)
{
    std::vector<std::size_t> row(b.size() + 1);
    for (std::size_t j = 0; j <= b.size(); ++j) {
        row[j] = j;
    }
    for (std::size_t i = 1; i <= a.size(); ++i) {
        std::size_t diag = row[0];
        row[0] = i;
        for (std::size_t j = 1; j <= b.size(); ++j) {
            const std::size_t up = row[j];
            row[j] = std::min({row[j] + 1, row[j - 1] + 1, diag + (a[i - 1] == b[j - 1] ? 0 : 1)});
            diag = up;
        }
    }
    return row[b.size()];
}

// `expected` is already normalized; allows ~1 OCR error per 4 characters.
bool fuzzyEquals(const std::string& ocrText, const std::string& expected)
{
    if (ocrText.empty() || expected.empty()) {
        return false;
    }
    return editDistance(normalizeWord(ocrText), expected) <= expected.size() / 4;
}

bool sameRowAndAdjacent(const Box& prev, const Box& next)
{
    const int prevBottom = prev.top + prev.height;
    const int nextBottom = next.top + next.height;
    const int overlap = std::min(prevBottom, nextBottom) - std::max(prev.top, next.top);
    const int minHeight = std::min(prev.height, next.height);
    const int maxHeight = std::max(prev.height, next.height);
    const int gap = next.left - (prev.left + prev.width);
    return overlap > minHeight / 2 && gap >= -maxHeight && gap <= 2 * maxHeight;
}

Box unite(const Box& a, const Box& b)
{
    const int left = std::min(a.left, b.left);
    const int top = std::min(a.top, b.top);
    const int right = std::max(a.left + a.width, b.left + b.width);
    const int bottom = std::max(a.top + a.height, b.top + b.height);
    return {left, top, right - left, bottom - top};
}

} // namespace

std::vector<OcrWord> parseTesseractTsv(std::string_view tsv, int scale)
{
    // Columns: level page block par line word left top width height conf text
    std::vector<OcrWord> words;
    if (scale < 1) {
        scale = 1;
    }
    for (auto line : split(tsv, '\n')) {
        if (!line.empty() && line.back() == '\r') {
            line.remove_suffix(1);
        }
        const auto cols = split(line, '\t');
        if (cols.size() < 12) {
            continue;
        }
        int level = 0;
        Box box;
        float conf = 0;
        if (!parseNumber(cols[0], level) || level != 5 || !parseNumber(cols[6], box.left)
            || !parseNumber(cols[7], box.top) || !parseNumber(cols[8], box.width)
            || !parseNumber(cols[9], box.height) || !parseNumber(cols[10], conf)) {
            continue;
        }
        std::string text(cols[11]);
        text.erase(0, text.find_first_not_of(' '));
        text.erase(text.find_last_not_of(' ') + 1);
        if (text.empty() || conf < 0) {
            continue;
        }
        box.left /= scale;
        box.top /= scale;
        box.width /= scale;
        box.height /= scale;
        words.push_back({std::move(text), box, conf});
    }
    return words;
}

std::string normalizeWord(std::string_view word)
{
    std::string out;
    for (std::size_t i = 0; i < word.size();) {
        const auto c = static_cast<unsigned char>(word[i]);
        if (c < 0x80) {
            if (std::isalnum(c)) {
                out.push_back(static_cast<char>(std::toupper(c)));
            }
            ++i;
            continue;
        }
        if (const char folded = foldHungarian(nextCodePoint(word, i))) {
            out.push_back(folded);
        }
    }
    return out;
}

std::optional<Box> findPhrase(const std::vector<OcrWord>& words, std::string_view phrase)
{
    std::vector<std::string> tokens;
    std::string joined;
    for (const auto part : split(phrase, ' ')) {
        auto token = normalizeWord(part);
        if (!token.empty()) {
            joined += token;
            tokens.push_back(std::move(token));
        }
    }
    if (tokens.empty()) {
        return std::nullopt;
    }

    for (std::size_t i = 0; i < words.size(); ++i) {
        // OCR sometimes glues the words of a button label together.
        if (tokens.size() > 1 && fuzzyEquals(words[i].text, joined)) {
            return words[i].box;
        }
        if (!fuzzyEquals(words[i].text, tokens[0])) {
            continue;
        }
        Box box = words[i].box;
        Box prev = box;
        std::size_t k = 1;
        for (; k < tokens.size() && i + k < words.size(); ++k) {
            const auto& next = words[i + k];
            if (!sameRowAndAdjacent(prev, next.box) || !fuzzyEquals(next.text, tokens[k])) {
                break;
            }
            box = unite(box, next.box);
            prev = next.box;
        }
        if (k == tokens.size()) {
            return box;
        }
    }
    return std::nullopt;
}

std::expected<std::vector<OcrWord>, std::string> recognize(const std::filesystem::path& image,
                                                           const std::string& language)
{
    const auto prepared = std::filesystem::temp_directory_path()
        / ("the_app_AI_ocr_" + std::to_string(getpid()) + ".png");

    auto conv = run({"convert", image.string(), "-colorspace", "Gray", "-resize",
                     std::to_string(kOcrScale * 100) + "%", prepared.string()});
    if (!conv) {
        return std::unexpected(conv.error());
    }
    if (conv->exitCode != 0) {
        return std::unexpected("convert failed with exit code " + std::to_string(conv->exitCode));
    }

    // --psm 11: sparse text, suited to UI screenshots with scattered labels.
    auto ocr = run({"tesseract", prepared.string(), "stdout", "-l", language, "--psm", "11", "tsv"});
    std::error_code ec;
    std::filesystem::remove(prepared, ec);
    if (!ocr) {
        return std::unexpected(ocr.error());
    }
    if (ocr->exitCode != 0) {
        return std::unexpected("tesseract failed with exit code " + std::to_string(ocr->exitCode)
                               + " (is tesseract-ocr-" + language + " installed?)");
    }
    return parseTesseractTsv(ocr->output, kOcrScale);
}

} // namespace app
