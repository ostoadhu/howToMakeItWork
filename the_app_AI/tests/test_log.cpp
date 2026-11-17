#include "log.hpp"

#include <gtest/gtest.h>

#include <fstream>
#include <regex>
#include <sstream>
#include <unistd.h>

TEST(Log, FormatLineHasTimestampLevelStateAndMessage)
{
    const auto line = app::log::formatLine(std::chrono::system_clock::now(), app::log::Level::Info,
                                           "Login", "login button clicked");
    EXPECT_TRUE(std::regex_match(
        line, std::regex(R"(\d{4}-\d\d-\d\d \d\d:\d\d:\d\d\.\d{3} \[INFO\] \[Login\] login button clicked)")))
        << line;
}

TEST(Log, FormatLineKeepsMilliseconds)
{
    const auto t = std::chrono::system_clock::time_point{} + std::chrono::milliseconds(42);
    const auto line = app::log::formatLine(t, app::log::Level::Error, "Error", "x");
    EXPECT_NE(line.find(".042 [ERROR] [Error] x"), std::string::npos) << line;
}

TEST(Log, WriteAppendsToLogFile)
{
    const auto path = std::filesystem::temp_directory_path()
        / ("the_app_AI_test_" + std::to_string(getpid()) + ".log");
    std::filesystem::remove(path);

    ASSERT_TRUE(app::log::open(path).has_value());
    app::log::info("Login", "first");
    app::log::warning("Login", "second");
    ASSERT_TRUE(app::log::open("/dev/null").has_value()); // close the test file

    std::ifstream in(path);
    std::stringstream content;
    content << in.rdbuf();
    const auto text = content.str();
    EXPECT_NE(text.find("[INFO] [Login] first\n"), std::string::npos) << text;
    EXPECT_NE(text.find("[WARN] [Login] second\n"), std::string::npos) << text;
    std::filesystem::remove(path);
}
