#include "browser.hpp"

#include <gtest/gtest.h>

TEST(Browser, WaitForWindowFailsIfNotStarted)
{
    const app::Browser browser;
    const auto res = browser.waitForWindow(std::chrono::seconds(1));
    ASSERT_FALSE(res.has_value());
    EXPECT_EQ(res.error(), "browser not started");
}
