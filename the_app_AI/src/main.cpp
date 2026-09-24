#include "browser.hpp"
#include "screenshot.hpp"
#include "session.hpp"

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <thread>

namespace {

constexpr auto kWindowTimeout = std::chrono::seconds(30);
constexpr auto kPageLoadDelay = std::chrono::seconds(5);

int fail(const std::string& msg)
{
    std::cerr << "error: " << msg << '\n';
    return EXIT_FAILURE;
}

} // namespace

int main(int argc, char* argv[])
{
    if (argc < 2 || argc > 3) {
        std::cerr << "usage: " << argv[0] << " <url> [output.png]\n";
        return EXIT_FAILURE;
    }
    const std::string url = argv[1];
    const std::filesystem::path output =
        std::filesystem::absolute(argc == 3 ? argv[2] : "screenshot.png");

    if (auto r = app::requireX11Session(); !r) {
        return fail(r.error());
    }
    std::cout << "session: " << app::toString(app::detectSession()) << '\n';

    app::Browser browser;
    if (auto r = browser.open(url); !r) {
        return fail("cannot launch chromium: " + r.error());
    }
    std::cout << "chromium launched: " << url << '\n';

    auto windowId = browser.waitForWindow(kWindowTimeout);
    if (!windowId) {
        return fail(windowId.error());
    }
    std::cout << "window id: " << *windowId << '\n';

    // Give the page time to load and render.
    std::this_thread::sleep_for(kPageLoadDelay);

    if (auto r = app::captureWindow(*windowId, output); !r) {
        return fail("screenshot failed: " + r.error());
    }
    std::cout << "screenshot saved: " << output.string() << '\n';

    if (auto r = app::openInGimp(output); !r) {
        return fail("cannot launch gimp: " + r.error());
    }
    std::cout << "opened in gimp\n";

    return EXIT_SUCCESS;
}
