#include "browser.hpp"
#include "input.hpp"
#include "ocr.hpp"
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
constexpr auto kDefaultUrl = "https://www.tippmixpro.hu";
constexpr auto kAfterClickDelay = std::chrono::seconds(2);
// Label of the "reject all" button of the tippmixpro.hu cookie banner.
constexpr auto kCookieRejectLabel = "ÖSSZES ELUTASÍTÁSA";

int fail(const std::string& msg)
{
    std::cerr << "error: " << msg << '\n';
    return EXIT_FAILURE;
}

// Reads the screenshot with OCR; if the cookie banner is visible, clicks its
// "reject all" button and re-captures the window. OCR problems are only
// warnings: the screenshot is still usable, just with the banner on it.
void dismissCookieBanner(const std::string& windowId, const std::filesystem::path& screenshot)
{
    auto words = app::recognize(screenshot);
    if (!words) {
        std::cerr << "warning: OCR failed, cookie banner not handled: " << words.error() << '\n';
        return;
    }
    const auto button = app::findPhrase(*words, kCookieRejectLabel);
    if (!button) {
        std::cout << "no cookie banner detected\n";
        return;
    }
    std::cout << "cookie banner found, clicking \"" << kCookieRejectLabel << "\" at "
              << button->centerX() << ',' << button->centerY() << '\n';

    if (auto r = app::clickInWindow(windowId, button->centerX(), button->centerY()); !r) {
        std::cerr << "warning: click failed: " << r.error() << '\n';
        return;
    }
    std::this_thread::sleep_for(kAfterClickDelay);

    if (auto r = app::captureWindow(windowId, screenshot); !r) {
        std::cerr << "warning: re-capture failed: " << r.error() << '\n';
        return;
    }
    auto after = app::recognize(screenshot);
    if (!after) {
        std::cerr << "warning: OCR of new screenshot failed: " << after.error() << '\n';
    } else if (app::findPhrase(*after, kCookieRejectLabel)) {
        std::cerr << "warning: cookie banner still visible after click\n";
    } else {
        std::cout << "cookie banner dismissed\n";
    }
}

} // namespace

int main(int argc, char* argv[])
{
    if (argc > 3) {
        std::cerr << "usage: " << argv[0] << " [url] [output.png]\n"
                  << "  default url: " << kDefaultUrl << '\n';
        return EXIT_FAILURE;
    }
    const std::string url = argc >= 2 ? argv[1] : kDefaultUrl;
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

    dismissCookieBanner(*windowId, output);

    if (auto r = app::openInGimp(output); !r) {
        return fail("cannot launch gimp: " + r.error());
    }
    std::cout << "opened in gimp\n";

    return EXIT_SUCCESS;
}
