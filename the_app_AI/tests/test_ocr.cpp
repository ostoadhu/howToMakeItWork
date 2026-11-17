#include "ocr.hpp"

#include <gtest/gtest.h>

namespace {

// Shaped like `tesseract shot.png stdout -l hun --psm 11 tsv` on the cookie banner.
constexpr const char* kBannerTsv =
    "level\tpage_num\tblock_num\tpar_num\tline_num\tword_num\tleft\ttop\twidth\theight\tconf\ttext\n"
    "1\t1\t0\t0\t0\t0\t0\t0\t2732\t1800\t-1\t\n"
    "5\t1\t1\t1\t1\t1\t634\t540\t138\t36\t91.2\tBeállítások\n"
    "5\t1\t2\t1\t1\t1\t1374\t534\t130\t34\t95.0\tÖSSZES\n"
    "5\t1\t2\t1\t1\t2\t1518\t534\t274\t34\t88.7\tELUTASlTÁSA\n"
    "5\t1\t3\t1\t1\t1\t1800\t534\t130\t34\t94.0\tÖSSZES\n"
    "5\t1\t3\t1\t1\t2\t1944\t534\t300\t34\t93.0\tENGEDÉLYEZÉSE\n";

} // namespace

TEST(Ocr, ParseKeepsOnlyWordsAndScalesCoordinates)
{
    const auto words = app::parseTesseractTsv(kBannerTsv, 2);
    ASSERT_EQ(words.size(), 5u);
    EXPECT_EQ(words[1].text, "ÖSSZES");
    EXPECT_EQ(words[1].box.left, 687);
    EXPECT_EQ(words[1].box.top, 267);
    EXPECT_EQ(words[1].box.width, 65);
    EXPECT_EQ(words[1].box.height, 17);
    EXPECT_FLOAT_EQ(words[1].confidence, 95.0f);
}

TEST(Ocr, ParseIgnoresGarbage)
{
    EXPECT_TRUE(app::parseTesseractTsv("").empty());
    EXPECT_TRUE(app::parseTesseractTsv("not\ta\ttsv\n").empty());
    EXPECT_TRUE(app::parseTesseractTsv("5\t1\t1\t1\t1\t1\t1\t1\t1\t1\t90\t   \n").empty());
}

TEST(Ocr, NormalizeFoldsHungarianAccents)
{
    EXPECT_EQ(app::normalizeWord("Összes,"), "OSSZES");
    EXPECT_EQ(app::normalizeWord("elutasítása"), "ELUTASITASA");
    EXPECT_EQ(app::normalizeWord("őűÁÉÍÓÖŐÚÜŰ"), "OUAEIOOOUUU");
    EXPECT_EQ(app::normalizeWord("\"!?"), "");
}

TEST(Ocr, FindPhraseLocatesRejectButtonDespiteOcrError)
{
    const auto words = app::parseTesseractTsv(kBannerTsv, 2);
    const auto box = app::findPhrase(words, "ÖSSZES ELUTASÍTÁSA");
    ASSERT_TRUE(box.has_value());
    EXPECT_EQ(box->left, 687);
    EXPECT_EQ(box->width, 896 - 687);
    EXPECT_EQ(box->centerX(), 687 + (896 - 687) / 2);
    EXPECT_EQ(box->centerY(), 267 + 17 / 2);
}

TEST(Ocr, FindPhraseDoesNotConfuseAcceptAndReject)
{
    const auto words = app::parseTesseractTsv(kBannerTsv, 2);
    const auto accept = app::findPhrase(words, "Összes engedélyezése");
    ASSERT_TRUE(accept.has_value());
    EXPECT_EQ(accept->left, 900);
}

TEST(Ocr, FindPhraseHandlesGluedWords)
{
    const std::vector<app::OcrWord> words{{"ÖSSZESELUTASÍTÁSA", {10, 20, 200, 18}, 80.0f}};
    const auto box = app::findPhrase(words, "ÖSSZES ELUTASÍTÁSA");
    ASSERT_TRUE(box.has_value());
    EXPECT_EQ(box->left, 10);
}

TEST(Ocr, FindPhraseRequiresSameRow)
{
    const std::vector<app::OcrWord> words{
        {"ÖSSZES", {10, 20, 60, 18}, 90.0f},
        {"ELUTASÍTÁSA", {75, 200, 120, 18}, 90.0f},
    };
    EXPECT_FALSE(app::findPhrase(words, "ÖSSZES ELUTASÍTÁSA").has_value());
}

TEST(Ocr, FindPhraseReturnsNulloptWhenAbsent)
{
    const auto words = app::parseTesseractTsv(kBannerTsv, 2);
    EXPECT_FALSE(app::findPhrase(words, "Bejelentkezés").has_value());
    EXPECT_FALSE(app::findPhrase(words, "").has_value());
}

TEST(Ocr, UpperRightCornerIsRightHalfAndTopFifth)
{
    const auto corner = app::upperRightCorner(1024, 768);
    EXPECT_EQ(corner.left, 512);
    EXPECT_EQ(corner.top, 0);
    EXPECT_EQ(corner.width, 512);
    EXPECT_EQ(corner.height, 153);
}

TEST(Ocr, WordsInRegionKeepsOnlyWordsCompletelyInside)
{
    const std::vector<app::OcrWord> words{
        {"inside", {900, 20, 80, 16}, 90.0f},
        {"left", {100, 20, 80, 16}, 90.0f},
        {"below", {900, 400, 80, 16}, 90.0f},
        {"crossing", {500, 20, 80, 16}, 90.0f},
    };
    const auto inside = app::wordsInRegion(words, app::upperRightCorner(1024, 768));
    ASSERT_EQ(inside.size(), 1u);
    EXPECT_EQ(inside[0].text, "inside");
}

TEST(Ocr, LoginButtonIsFoundOnlyInUpperRightCorner)
{
    const auto corner = app::upperRightCorner(1024, 768);
    const std::vector<app::OcrWord> words{
        {"Bejelentkezés", {40, 600, 110, 16}, 90.0f},   // e.g. a footer link
        {"Regisztráció", {800, 24, 90, 16}, 90.0f},
        {"Bejelentkezes", {910, 24, 100, 16}, 85.0f},   // OCR lost the accent
    };
    const auto button = app::findPhrase(app::wordsInRegion(words, corner), "Bejelentkezés");
    ASSERT_TRUE(button.has_value());
    EXPECT_EQ(button->left, 910);
    EXPECT_EQ(button->centerX(), 960);
    EXPECT_EQ(button->centerY(), 32);
}

TEST(Ocr, LoginButtonOutsideCornerIsNotFound)
{
    const std::vector<app::OcrWord> words{{"Bejelentkezés", {40, 600, 110, 16}, 90.0f}};
    EXPECT_TRUE(app::findPhrase(words, "Bejelentkezés").has_value());
    EXPECT_FALSE(app::findPhrase(app::wordsInRegion(words, app::upperRightCorner(1024, 768)),
                                 "Bejelentkezés")
                     .has_value());
}

TEST(Ocr, FuzzyMatchesTypedEmailAddress)
{
    EXPECT_TRUE(app::fuzzyMatches("ostorhazyadam@gmail.com", "ostorhazyadam@gmail.com"));
    // OCR noise: '@' read as '', 'l' as '1'.
    EXPECT_TRUE(app::fuzzyMatches("ostorhazyadam©gmai1.com", "ostorhazyadam@gmail.com"));
    EXPECT_FALSE(app::fuzzyMatches("", "ostorhazyadam@gmail.com"));
    EXPECT_FALSE(app::fuzzyMatches("someone@example.com", "ostorhazyadam@gmail.com"));
    EXPECT_FALSE(app::fuzzyMatches("ostorhazy", "ostorhazyadam@gmail.com"));
}

namespace {
app::OcrWord word(std::string text, int left, int top, int width, int height = 16)
{
    return {std::move(text), {left, top, width, height}, 90.0f};
}
} // namespace

TEST(Ocr, GroupsHorizontalListIntoItems)
{
    // "Sport   Élő fogadás   Kaszinó" - words of an item 5 px apart, items 40 px apart.
    const std::vector<app::OcrWord> words{
        word("Kaszinó", 230, 20, 70),
        word("Sport", 10, 21, 50),
        word("Élő", 100, 20, 30),
        word("fogadás", 135, 19, 55),
    };
    const auto items = app::groupListItems(words);
    ASSERT_EQ(items.size(), 3u);
    EXPECT_EQ(items[0].text, "Sport");
    EXPECT_EQ(items[1].text, "Élő fogadás");
    EXPECT_EQ(items[1].box.left, 100);
    EXPECT_EQ(items[1].box.width, 90);
    EXPECT_EQ(items[2].text, "Kaszinó");
}

TEST(Ocr, GroupListItemsSplitsRowsAndDropsIconNoise)
{
    const std::vector<app::OcrWord> words{
        word("Foci", 10, 60, 40),
        word("|", 70, 20, 4),
        word("Lottó", 10, 20, 45),
        word("", 90, 60, 6),
        word("Tenisz", 100, 61, 55),
    };
    const auto items = app::groupListItems(words);
    ASSERT_EQ(items.size(), 3u);
    EXPECT_EQ(items[0].text, "Lottó");
    EXPECT_EQ(items[1].text, "Foci");
    EXPECT_EQ(items[2].text, "Tenisz");
}

TEST(Ocr, GroupListItemsOfNothingIsEmpty)
{
    EXPECT_TRUE(app::groupListItems({}).empty());
}