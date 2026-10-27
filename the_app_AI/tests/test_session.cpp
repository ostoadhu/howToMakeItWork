#include "session.hpp"

#include <gtest/gtest.h>

#include <cstdlib>
#include <iterator>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace {

// Saves the session environment variables before each test and restores them
// afterwards, so tests can freely modify them without leaking state.
class SessionTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        for (const char* name : kVars) {
            const char* value = std::getenv(name);
            saved_.push_back(value ? std::optional<std::string>(value) : std::nullopt);
            unsetenv(name);
        }
    }

    void TearDown() override
    {
        for (std::size_t i = 0; i < std::size(kVars); ++i) {
            if (saved_[i]) {
                setenv(kVars[i], saved_[i]->c_str(), 1);
            } else {
                unsetenv(kVars[i]);
            }
        }
    }

    static constexpr const char* kVars[] = {"XDG_SESSION_TYPE", "WAYLAND_DISPLAY", "DISPLAY"};

private:
    std::vector<std::optional<std::string>> saved_;
};

} // namespace

TEST_F(SessionTest, UnknownWhenNoVariablesSet)
{
    EXPECT_EQ(app::detectSession(), app::SessionType::Unknown);
}

TEST_F(SessionTest, X11FromXdgSessionType)
{
    setenv("XDG_SESSION_TYPE", "x11", 1);
    EXPECT_EQ(app::detectSession(), app::SessionType::X11);
}

TEST_F(SessionTest, X11FromDisplayOnly)
{
    setenv("DISPLAY", ":0", 1);
    EXPECT_EQ(app::detectSession(), app::SessionType::X11);
}

TEST_F(SessionTest, WaylandFromXdgSessionType)
{
    setenv("XDG_SESSION_TYPE", "wayland", 1);
    EXPECT_EQ(app::detectSession(), app::SessionType::Wayland);
}

TEST_F(SessionTest, WaylandDisplayWinsOverX11Display)
{
    // XWayland sets DISPLAY too; WAYLAND_DISPLAY must take precedence.
    setenv("DISPLAY", ":0", 1);
    setenv("WAYLAND_DISPLAY", "wayland-0", 1);
    EXPECT_EQ(app::detectSession(), app::SessionType::Wayland);
}

TEST_F(SessionTest, RequireX11SucceedsOnX11)
{
    setenv("XDG_SESSION_TYPE", "x11", 1);
    EXPECT_TRUE(app::requireX11Session().has_value());
}

TEST_F(SessionTest, RequireX11FailsOnWayland)
{
    setenv("XDG_SESSION_TYPE", "wayland", 1);
    const auto r = app::requireX11Session();
    ASSERT_FALSE(r.has_value());
    EXPECT_NE(r.error().find("Wayland"), std::string::npos);
}

TEST_F(SessionTest, RequireX11FailsWithoutDisplay)
{
    const auto r = app::requireX11Session();
    ASSERT_FALSE(r.has_value());
    EXPECT_NE(r.error().find("DISPLAY"), std::string::npos);
}

// Parameterized test: one test body, run once per (input, expected) pair.
class SessionToStringTest
    : public ::testing::TestWithParam<std::pair<app::SessionType, std::string>> {};

TEST_P(SessionToStringTest, ReturnsLowercaseName)
{
    const auto& [type, expected] = GetParam();
    EXPECT_EQ(app::toString(type), expected);
}

INSTANTIATE_TEST_SUITE_P(AllTypes, SessionToStringTest,
                         ::testing::Values(std::pair{app::SessionType::X11, std::string("x11")},
                                           std::pair{app::SessionType::Wayland, std::string("wayland")},
                                           std::pair{app::SessionType::Unknown, std::string("unknown")}));
