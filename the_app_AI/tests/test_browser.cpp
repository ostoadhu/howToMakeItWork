#include "browser.hpp"

#include <gtest/gtest.h>

#include <algorithm>

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

TEST(Browser, ChromiumOpensUrlInIncognitoWindowAtGivenSize)
{
    const auto cmd = app::chromiumCommand("https://www.tippmixpro.hu", {1024, 768}, "/tmp/profile_1");
    ASSERT_FALSE(cmd.empty());
    EXPECT_EQ(cmd.front(), "chromium");
    auto has = [&](const std::string& arg) { return std::ranges::find(cmd, arg) != cmd.end(); };
    EXPECT_TRUE(has("--incognito"));
    EXPECT_TRUE(has("--window-size=1024,768"));
    EXPECT_TRUE(has("--user-data-dir=/tmp/profile_1"));
    EXPECT_TRUE(has("--ozone-platform=x11"));
    EXPECT_TRUE(std::ranges::none_of(cmd, [](const std::string& a) { return a.starts_with("--app"); }));
    // The url is last, after "--", so it can never be taken for a switch.
    ASSERT_GE(cmd.size(), 2u);
    EXPECT_EQ(cmd[cmd.size() - 2], "--");
    EXPECT_EQ(cmd.back(), "https://www.tippmixpro.hu");
}