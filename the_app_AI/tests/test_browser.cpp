#include "browser.hpp"

#include <gtest/gtest.h>

TEST(Browser, ParseWindowGeometry)
{
    const auto size = app::parseWindowGeometry(
        "WINDOW=12345\nX=0\nY=0\nWIDTH=1024\nHEIGHT=768\nSCREEN=0\n");
    ASSERT_TRUE(size.has_value());
    EXPECT_EQ(*size, (app::WindowSize{1024, 768}));
}

TEST(Browser, ParseWindowGeometryRejectsMissingOrInvalidValues)
{
    EXPECT_FALSE(app::parseWindowGeometry("").has_value());
    EXPECT_FALSE(app::parseWindowGeometry("WIDTH=1024\n").has_value());
    EXPECT_FALSE(app::parseWindowGeometry("WIDTH=abc\nHEIGHT=768\n").has_value());
    EXPECT_FALSE(app::parseWindowGeometry("WIDTH=0\nHEIGHT=768\n").has_value());
}

TEST(Browser, WindowSizeToString)
{
    EXPECT_EQ(app::toString(app::WindowSize{1024, 768}), "1024x768");
}

TEST(Browser, WaitForWindowFailsIfNotStarted)
{
    const app::Browser browser;
    const auto res = browser.waitForWindow(std::chrono::seconds(1));
    ASSERT_FALSE(res.has_value());
    EXPECT_EQ(res.error(), "browser not started");
}
