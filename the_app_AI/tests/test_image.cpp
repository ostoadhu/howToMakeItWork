#include "image.hpp"

#include <gtest/gtest.h>

#include <string>

namespace {

constexpr std::uint8_t kDark = 30;
constexpr std::uint8_t kLight = 220;

app::GrayImage blank(int w, int h, std::uint8_t value = kDark)
{
    return {w, h, std::vector<std::uint8_t>(static_cast<std::size_t>(w) * h, value)};
}

void fill(app::GrayImage& img, const app::Box& b, std::uint8_t value)
{
    for (int y = b.top; y < b.top + b.height; ++y) {
        for (int x = b.left; x < b.left + b.width; ++x) {
            img.pixels[static_cast<std::size_t>(y) * img.width + x] = value;
        }
    }
}

// Outlined field, `border` px thick; returns the inside box.
app::Box outline(app::GrayImage& img, const app::Box& outer, int border = 1)
{
    fill(img, outer, kLight);
    const app::Box inner{outer.left + border, outer.top + border, outer.width - 2 * border,
                         outer.height - 2 * border};
    fill(img, inner, kDark);
    return inner;
}

void disc(app::GrayImage& img, int cx, int cy, int r)
{
    for (int y = cy - r; y <= cy + r; ++y) {
        for (int x = cx - r; x <= cx + r; ++x) {
            if ((x - cx) * (x - cx) + (y - cy) * (y - cy) <= r * r) {
                img.pixels[static_cast<std::size_t>(y) * img.width + x] = kLight;
            }
        }
    }
}

// Login popup like on tippmixpro.hu: label above an outlined field.
struct Popup {
    app::GrayImage image = blank(400, 200);
    app::Box label{20, 20, 120, 14};
    app::Box field;

    Popup()
    {
        // "Text" of the label: vertical strokes, one with a descender below the box.
        for (int x = label.left; x < label.left + label.width; x += 6) {
            fill(image, {x, label.top, 2, label.height}, kLight);
        }
        fill(image, {label.left + 60, label.top + label.height, 2, 3}, kLight);
        field = outline(image, {20, 44, 300, 40}, 2);
    }
};

} // namespace

TEST(Image, ParsesBinaryPgm)
{
    std::string data = "P5\n# comment\n3 2\n255\n";
    data += std::string{'\x00', '\x10', '\x20', '\x30', '\x40', '\xff'};
    const auto img = app::parsePgm(data);
    ASSERT_TRUE(img.has_value()) << img.error();
    EXPECT_EQ(img->width, 3);
    EXPECT_EQ(img->height, 2);
    EXPECT_EQ(img->at(0, 0), 0x00);
    EXPECT_EQ(img->at(2, 0), 0x20);
    EXPECT_EQ(img->at(2, 1), 0xff);
}

TEST(Image, RejectsInvalidPgm)
{
    EXPECT_FALSE(app::parsePgm("P2\n1 1\n255\n0").has_value());
    EXPECT_FALSE(app::parsePgm("P5\n2 2\n255\n").has_value());
    EXPECT_FALSE(app::parsePgm("P5\n2 2\n65535\n12345678").has_value());
    EXPECT_FALSE(app::parsePgm("P5\nx 2\n255\n1234").has_value());
}

TEST(Image, FindsOutlinedFieldBelowLabel)
{
    const Popup p;
    const auto field = app::findInputBelow(p.image, p.label);
    ASSERT_TRUE(field.has_value());
    EXPECT_EQ(field->left, p.field.left);
    EXPECT_EQ(field->top, p.field.top);
    EXPECT_EQ(field->width, p.field.width);
    EXPECT_EQ(field->height, p.field.height);
}

TEST(Image, TextInsideTheFieldIsNotTakenForTheBorder)
{
    Popup p;
    // Typed text in the field, right below the label column (glyph strokes, not a solid bar).
    for (int x = p.field.left + 40; x < p.field.left + 160; x += 5) {
        fill(p.image, {x, p.field.top + 10, 2, 16}, kLight);
    }
    const auto field = app::findInputBelow(p.image, p.label);
    ASSERT_TRUE(field.has_value());
    EXPECT_EQ(field->height, p.field.height);
    EXPECT_EQ(field->width, p.field.width);
}

TEST(Image, NoFieldBelowLabel)
{
    Popup p;
    fill(p.image, {0, 40, 400, 160}, kDark);
    EXPECT_FALSE(app::findInputBelow(p.image, p.label).has_value());
}

TEST(Image, FieldTooFarBelowLabelIsIgnored)
{
    auto img = blank(400, 300);
    const app::Box label{20, 20, 120, 14};
    outline(img, {20, 200, 300, 40});
    EXPECT_FALSE(app::findInputBelow(img, label, 80).has_value());
    EXPECT_TRUE(app::findInputBelow(img, label, 200).has_value());
}

TEST(Image, ButtonLikeShapeIsNotAField)
{
    auto img = blank(400, 200);
    const app::Box label{20, 20, 60, 14};
    outline(img, {20, 44, 60, 40}); // narrower than 3x its height
    EXPECT_FALSE(app::findInputBelow(img, label).has_value());
}

TEST(Image, FindsPasswordDots)
{
    Popup p;
    for (int i = 0; i < 8; ++i) {
        disc(p.image, p.field.left + 20 + i * 14, p.field.centerY(), 4);
    }
    const auto dots = app::findDots(p.image, p.field);
    EXPECT_EQ(dots.size(), 8u);
}

TEST(Image, CaretAndBordersAreNotDots)
{
    Popup p;
    fill(p.image, {p.field.left + 10, p.field.top + 8, 1, 20}, kLight); // text cursor
    EXPECT_TRUE(app::findDots(p.image, p.field).empty());
    // Whole outer box: the border touches the region edge.
    EXPECT_TRUE(app::findDots(p.image, {20, 44, 300, 40}).empty());
}

TEST(Image, CountsOnlyNewDotsIgnoringTheEyeIcon)
{
    Popup p;
    disc(p.image, p.field.left + p.field.width - 20, p.field.centerY(), 6); // eye icon
    const auto before = app::findDots(p.image, p.field);
    ASSERT_EQ(before.size(), 1u);

    for (int i = 0; i < 8; ++i) {
        disc(p.image, p.field.left + 20 + i * 14, p.field.centerY(), 4);
    }
    const auto after = app::findDots(p.image, p.field);
    EXPECT_EQ(after.size(), 9u);
    EXPECT_EQ(app::countNewDots(before, after), 8);
}

TEST(Image, CountNewDotsIgnoresOddBlobs)
{
    const std::vector<app::Box> after{
        {10, 10, 8, 8}, {24, 10, 8, 8}, {38, 10, 8, 8}, // a row of dots
        {60, 30, 8, 8},                                 // another row
        {80, 10, 16, 16},                               // different size
    };
    EXPECT_EQ(app::countNewDots({}, after), 3);
    EXPECT_EQ(app::countNewDots({}, {}), 0);
}
