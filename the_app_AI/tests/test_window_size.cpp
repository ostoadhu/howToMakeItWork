// Integration test for UseCase_3: the browser window must open at 1600x768.
// It launches a real chromium and reads the window size with xdotool, so it
// is skipped when no X11 session, chromium or xdotool is available.

#include "browser.hpp"
#include "process.hpp"
#include "session.hpp"

#include <gtest/gtest.h>

#include <csignal>
#include <filesystem>
#include <map>
#include <sstream>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>

namespace {

constexpr int kExpectedWidth = 1600;
constexpr int kExpectedHeight = 768;

bool toolAvailable(const std::string& tool)
{
    const auto res = app::run({"sh", "-c", "command -v \"$0\" >/dev/null", tool});
    return res && res->exitCode == 0;
}

// Parses `xdotool getwindowgeometry --shell` into KEY -> value.
std::map<std::string, std::string> parseShellOutput(const std::string& output)
{
    std::map<std::string, std::string> values;
    std::istringstream lines(output);
    std::string line;
    while (std::getline(lines, line)) {
        const auto eq = line.find('=');
        if (eq != std::string::npos) {
            values[line.substr(0, eq)] = line.substr(eq + 1);
        }
    }
    return values;
}

// Closes chromium (it runs in its own session) and removes its temporary profile.
struct BrowserCleanup {
    const app::Browser& browser;

    ~BrowserCleanup()
    {
        if (const pid_t pid = browser.pid(); pid > 0) {
            kill(-pid, SIGTERM);
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            kill(-pid, SIGKILL);
            waitpid(pid, nullptr, 0);
        }
        std::error_code ec;
        std::filesystem::remove_all("/tmp/the_app_AI_profile_" + std::to_string(getpid()), ec);
    }
};

} // namespace

TEST(BrowserWindow, OpensInPredefinedWindowSize)
{
    if (auto r = app::requireX11Session(); !r) {
        GTEST_SKIP() << r.error();
    }
    for (const auto* tool : {"chromium", "xdotool"}) {
        if (!toolAvailable(tool)) {
            GTEST_SKIP() << tool << " not found in PATH";
        }
    }

    const app::WindowSize size{kExpectedWidth, kExpectedHeight};
    app::Browser browser;
    const BrowserCleanup cleanup{browser};

    ASSERT_TRUE(browser.open("about:blank", size).has_value());
    const auto windowId = browser.waitForWindow(std::chrono::seconds(30));
    ASSERT_TRUE(windowId.has_value()) << windowId.error();

    const auto ensured = app::ensureWindowSize(*windowId, size);
    ASSERT_TRUE(ensured.has_value()) << ensured.error();

    const auto geometry = app::run({"xdotool", "getwindowgeometry", "--shell", *windowId});
    ASSERT_TRUE(geometry.has_value()) << geometry.error();
    ASSERT_EQ(geometry->exitCode, 0) << "xdotool getwindowgeometry failed";

    const auto values = parseShellOutput(geometry->output);
    ASSERT_TRUE(values.contains("WIDTH")) << geometry->output;
    ASSERT_TRUE(values.contains("HEIGHT")) << geometry->output;
    EXPECT_EQ(values.at("WIDTH"), std::to_string(kExpectedWidth));
    EXPECT_EQ(values.at("HEIGHT"), std::to_string(kExpectedHeight));
}
