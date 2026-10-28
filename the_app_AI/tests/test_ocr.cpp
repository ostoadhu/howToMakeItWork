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
