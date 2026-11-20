#include "browser.hpp"
#include "credentials.hpp"
#include "image.hpp"
#include "input.hpp"
#include "log.hpp"
#include "ocr.hpp"
#include "region_config.hpp"
#include "screenshot.hpp"
#include "session.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdlib>
#include <expected>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

namespace {

constexpr auto kWindowTimeout = std::chrono::seconds(30);
constexpr auto kPageLoadDelay = std::chrono::seconds(5);
constexpr auto kDefaultUrl = "https://www.tippmixpro.hu";
constexpr auto kAfterClickDelay = std::chrono::seconds(2);
constexpr app::WindowSize kWindowSize{1600, 768};
constexpr auto kLogFile = "the_app_AI.log";
// Label of the "reject all" button of the tippmixpro.hu cookie banner.
constexpr auto kCookieRejectLabel = "ÖSSZES ELUTASÍTÁSA";
// Label of the login button in the upper right corner of tippmixpro.hu.
constexpr auto kLoginLabel = "Bejelentkezés";
// Labels above the fields of the login popup (tried in order).
constexpr std::array kEmailLabels{"Felhasználónév vagy e-mail cím", "e-mail cím"};
constexpr auto kPasswordLabel = "Jelszó";
constexpr auto kAfterTypeDelay = std::chrono::milliseconds(700);
// How long to wait for the login popup to disappear after Enter, and how often to look.
constexpr auto kLoginTimeout = std::chrono::seconds(10);
constexpr auto kLoginPollInterval = std::chrono::seconds(1);

// State names, as in docs/state_machine.yaml.
constexpr auto kOpenWindow = "OpenWindow";
constexpr auto kWaitPageLoad = "WaitPageLoad";
constexpr auto kRejectCookies = "RejectCookies";
constexpr auto kLogin = "Login";
constexpr auto kEnterCredentials = "EnterCredentials";
constexpr auto kReadGameList = "ReadGameList";
constexpr auto kError = "Error";

int fail(const std::string& state, const std::string& msg)
{
    app::log::error(state, msg);
    app::log::error(kError, "exiting");
    return EXIT_FAILURE;
}

std::string describe(const app::Box& box)
{
    return "click point " + std::to_string(box.centerX()) + ',' + std::to_string(box.centerY())
        + ", box " + std::to_string(box.width) + 'x' + std::to_string(box.height) + " at "
        + std::to_string(box.left) + ',' + std::to_string(box.top);
}

// Reads the screenshot with OCR; if the cookie banner is visible, clicks its
// "reject all" button and re-captures the window. OCR problems are only
// warnings: the screenshot is still usable, just with the banner on it.
void dismissCookieBanner(const std::string& windowId, const std::filesystem::path& screenshot)
{
    auto words = app::recognize(screenshot);
    if (!words) {
        app::log::warning(kRejectCookies, "OCR failed, cookie banner not handled: " + words.error());
        return;
    }
    const auto button = app::findPhrase(*words, kCookieRejectLabel);
    if (!button) {
        app::log::info(kRejectCookies, "no cookie banner detected");
        return;
    }
    app::log::info(kRejectCookies, std::string("cookie banner found, clicking \"") + kCookieRejectLabel
                                       + "\": " + describe(*button));

    if (auto r = app::clickInWindow(windowId, button->centerX(), button->centerY()); !r) {
        app::log::warning(kRejectCookies, "click failed: " + r.error());
        return;
    }
    std::this_thread::sleep_for(kAfterClickDelay);

    if (auto r = app::captureWindow(windowId, screenshot); !r) {
        app::log::warning(kRejectCookies, "re-capture failed: " + r.error());
        return;
    }
    auto after = app::recognize(screenshot);
    if (!after) {
        app::log::warning(kRejectCookies, "OCR of new screenshot failed: " + after.error());
    } else if (app::findPhrase(*after, kCookieRejectLabel)) {
        app::log::warning(kRejectCookies, "cookie banner still visible after click");
    } else {
        app::log::info(kRejectCookies, "cookie banner dismissed");
    }
}

// UseCase_5: finds the login button in the upper right corner and clicks it.
std::expected<void, std::string> clickLoginButton(const std::string& windowId,
                                                  const std::filesystem::path& screenshot)
{
    if (auto r = app::captureWindow(windowId, screenshot); !r) {
        return std::unexpected("capture failed: " + r.error());
    }
    auto words = app::recognize(screenshot);
    if (!words) {
        return std::unexpected("OCR failed: " + words.error());
    }

    const auto corner = app::upperRightCorner(kWindowSize.width, kWindowSize.height);
    app::log::info(kLogin, std::string("searching \"") + kLoginLabel + "\" in the upper right corner ("
                               + std::to_string(corner.width) + 'x' + std::to_string(corner.height)
                               + " at " + std::to_string(corner.left) + ','
                               + std::to_string(corner.top) + ')');
    const auto button = app::findPhrase(app::wordsInRegion(*words, corner), kLoginLabel);
    if (!button) {
        if (app::findPhrase(*words, kLoginLabel)) {
            app::log::warning(kLogin, std::string("\"") + kLoginLabel
                                          + "\" found on the page, but not in the upper right corner");
        }
        return std::unexpected(std::string("login button \"") + kLoginLabel
                               + "\" not found in the upper right corner");
    }
    app::log::info(kLogin, std::string("login button \"") + kLoginLabel + "\" found: " + describe(*button));

    if (auto r = app::clickInWindow(windowId, button->centerX(), button->centerY()); !r) {
        return std::unexpected("click on login button failed: " + r.error());
    }
    app::log::info(kLogin, "login button clicked");
    std::this_thread::sleep_for(kAfterClickDelay);

    if (auto r = app::captureWindow(windowId, screenshot); !r) {
        app::log::warning(kLogin, "re-capture after click failed: " + r.error());
    } else {
        app::log::info(kLogin, "screenshot after click saved: " + screenshot.string());
    }
    return {};
}

// Screenshot + OCR + grayscale pixels of the current window content.
struct Snapshot {
    std::vector<app::OcrWord> words;
    app::GrayImage image;
};

std::expected<Snapshot, std::string> snapshot(const std::string& windowId,
                                              const std::filesystem::path& screenshot)
{
    if (auto r = app::captureWindow(windowId, screenshot); !r) {
        return std::unexpected("capture failed: " + r.error());
    }
    auto words = app::recognize(screenshot);
    if (!words) {
        return std::unexpected("OCR failed: " + words.error());
    }
    auto image = app::loadGray(screenshot);
    if (!image) {
        return std::unexpected("cannot read screenshot pixels: " + image.error());
    }
    return Snapshot{std::move(*words), std::move(*image)};
}

// Locates the input field below a label; `below` restricts the search to labels under that y.
std::expected<app::Box, std::string> locateField(const Snapshot& shot, std::string_view label,
                                                 int below = 0)
{
    std::vector<app::OcrWord> candidates;
    for (const auto& w : shot.words) {
        if (w.box.top >= below) {
            candidates.push_back(w);
        }
    }
    const auto labelBox = app::findPhrase(candidates, label);
    if (!labelBox) {
        return std::unexpected("label \"" + std::string(label) + "\" not found");
    }
    const auto field = app::findInputBelow(shot.image, *labelBox);
    if (!field) {
        return std::unexpected("no input field found below label \"" + std::string(label)
                               + "\" (" + describe(*labelBox) + ')');
    }
    return *field;
}

// Text fields: click in the left part, away from icons such as the password "eye".
int clickX(const app::Box& field)
{
    return field.left + std::min(field.width / 4, 120);
}

// The login popup is visible while its field labels can be read on the screenshot.
bool loginPopupVisible(const std::vector<app::OcrWord>& words)
{
    for (const auto* label : kEmailLabels) {
        if (app::findPhrase(words, label)) {
            return true;
        }
    }
    return app::findPhrase(words, kPasswordLabel).has_value();
}

// UseCase_7: fills in and validates the e-mail address and the password of the login popup.
std::expected<void, std::string> enterCredentials(const std::string& windowId,
                                                  const std::filesystem::path& screenshot,
                                                  const app::Credentials& credentials)
{
    auto shot = snapshot(windowId, screenshot);
    if (!shot) {
        return std::unexpected(shot.error());
    }

    std::expected<app::Box, std::string> emailField = std::unexpected("no e-mail label configured");
    for (const auto* label : kEmailLabels) {
        emailField = locateField(*shot, label);
        if (emailField) {
            app::log::info(kEnterCredentials, std::string("e-mail field found below \"") + label
                                                  + "\": " + describe(*emailField));
            break;
        }
    }
    if (!emailField) {
        return std::unexpected("e-mail field: " + emailField.error());
    }
    auto passwordField = locateField(*shot, kPasswordLabel, emailField->top + emailField->height);
    if (!passwordField) {
        return std::unexpected("password field: " + passwordField.error());
    }
    app::log::info(kEnterCredentials, std::string("password field found below \"") + kPasswordLabel
                                          + "\": " + describe(*passwordField));

    // E-mail address: click, type, validate with OCR.
    if (auto r = app::clickInWindow(windowId, clickX(*emailField), emailField->centerY()); !r) {
        return std::unexpected("click on e-mail field failed: " + r.error());
    }
    if (auto r = app::typeText(windowId, credentials.email); !r) {
        return std::unexpected("typing the e-mail address failed: " + r.error());
    }
    app::log::info(kEnterCredentials, "e-mail address typed: " + credentials.email);
    std::this_thread::sleep_for(kAfterTypeDelay);

    shot = snapshot(windowId, screenshot);
    if (!shot) {
        return std::unexpected(shot.error());
    }
    std::string emailText;
    const app::Box textArea{emailField->left - 4, emailField->top - 4, emailField->width + 8,
                            emailField->height + 8};
    for (const auto& w : app::wordsInRegion(shot->words, textArea)) {
        emailText += w.text;
    }
    if (!app::fuzzyMatches(emailText, credentials.email)) {
        return std::unexpected("e-mail field validation failed: OCR reads \"" + emailText + '"');
    }
    app::log::info(kEnterCredentials, "e-mail field validated: OCR reads \"" + emailText + '"');

    // Password: the field is empty (no dots) before typing. Remember the dot-like shapes
    // already there (e.g. the eye icon) so that only the dots of the typed password are counted.
    const auto dotsBefore = app::findDots(shot->image, *passwordField);
    if (auto r = app::clickInWindow(windowId, clickX(*passwordField), passwordField->centerY()); !r) {
        return std::unexpected("click on password field failed: " + r.error());
    }
    if (auto r = app::typeText(windowId, credentials.password); !r) {
        return std::unexpected("typing the password failed: " + r.error());
    }
    app::log::info(kEnterCredentials,
                   "password typed (" + std::to_string(credentials.password.size()) + " characters)");
    std::this_thread::sleep_for(kAfterTypeDelay);

    if (auto r = app::captureWindow(windowId, screenshot); !r) {
        return std::unexpected("capture failed: " + r.error());
    }
    auto image = app::loadGray(screenshot);
    if (!image) {
        return std::unexpected("cannot read screenshot pixels: " + image.error());
    }
    const int dots = app::countNewDots(dotsBefore, app::findDots(*image, *passwordField));
    const int expected = static_cast<int>(credentials.password.size());
    if (dots != expected) {
        return std::unexpected("password field validation failed: " + std::to_string(dots)
                               + " dots instead of " + std::to_string(expected));
    }
    app::log::info(kEnterCredentials, "password field validated: " + std::to_string(dots) + " dots");

    // Submit with Enter; the login succeeded when the popup disappears.
    if (auto r = app::pressKey(windowId, "Return"); !r) {
        return std::unexpected("pressing Enter failed: " + r.error());
    }
    app::log::info(kEnterCredentials, "Enter pressed, waiting for the login popup to disappear");

    const auto deadline = std::chrono::steady_clock::now() + kLoginTimeout;
    for (;;) {
        std::this_thread::sleep_for(kLoginPollInterval);
        if (auto r = app::captureWindow(windowId, screenshot); !r) {
            return std::unexpected("capture failed: " + r.error());
        }
        auto words = app::recognize(screenshot);
        if (!words) {
            return std::unexpected("OCR failed: " + words.error());
        }
        if (!loginPopupVisible(*words)) {
            app::log::info(kEnterCredentials, "login popup disappeared: login successful");
            return {};
        }
        if (std::chrono::steady_clock::now() >= deadline) {
            return std::unexpected("login failed: popup still visible "
                                   + std::to_string(kLoginTimeout.count())
                                   + " s after Enter (screenshot: " + screenshot.string() + ')');
        }
    }
}

// UseCase_8: reads the names of the games listed horizontally in the configured region.
std::expected<std::vector<app::ListItem>, std::string> readGameList(const std::string& windowId,
                                                                   const std::filesystem::path& screenshot,
                                                                   const app::Box& region)
{
    if (auto r = app::captureWindow(windowId, screenshot); !r) {
        return std::unexpected("capture failed: " + r.error());
    }
    app::log::info(kReadGameList, "reading the game list in " + std::string(app::kGameListRegion) + " ("
                                      + std::to_string(region.width) + 'x' + std::to_string(region.height)
                                      + " at " + std::to_string(region.left) + ','
                                      + std::to_string(region.top) + ')');
    auto words = app::recognizeRegion(screenshot, region);
    if (!words) {
        return std::unexpected("OCR failed: " + words.error());
    }
    auto games = app::groupListItems(std::move(*words));
    if (games.empty()) {
        return std::unexpected("no game found in " + std::string(app::kGameListRegion)
                               + " (screenshot: " + screenshot.string() + ')');
    }
    app::log::info(kReadGameList, std::to_string(games.size()) + " games found");
    for (std::size_t i = 0; i < games.size(); ++i) {
        app::log::info(kReadGameList, "game " + std::to_string(i + 1) + ": \"" + games[i].text
                                          + "\", " + describe(games[i].box));
    }
    return games;
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

    if (auto r = app::log::open(kLogFile); !r) {
        std::cerr << "warning: " << r.error() << ", logging to the terminal only\n";
    }

    const auto credentials = app::loadCredentials(app::credentialsPath());
    if (!credentials) {
        return fail(kOpenWindow, "credentials: " + credentials.error());
    }

    const auto regions = app::loadRegionConfig(app::regionConfigPath());
    if (!regions) {
        return fail(kOpenWindow, "region config: " + regions.error());
    }
    const auto gameListRegion =
        app::requireRegion(*regions, app::kGameListRegion, kWindowSize.width, kWindowSize.height);
    if (!gameListRegion) {
        return fail(kOpenWindow, "region config: " + gameListRegion.error());
    }

    if (auto r = app::requireX11Session(); !r) {
        return fail(kOpenWindow, r.error());
    }
    app::log::info(kOpenWindow, "session: " + app::toString(app::detectSession()));

    app::Browser browser;
    if (auto r = browser.open(url, kWindowSize); !r) {
        return fail(kOpenWindow, "cannot launch chromium: " + r.error());
    }
    app::log::info(kOpenWindow, "chromium launched: " + url);

    auto windowId = browser.waitForWindow(kWindowTimeout);
    if (!windowId) {
        return fail(kOpenWindow, windowId.error());
    }
    app::log::info(kOpenWindow, "window id: " + *windowId);

    if (auto r = app::ensureWindowSize(*windowId, kWindowSize); !r) {
        return fail(kOpenWindow, "cannot set window size: " + r.error());
    }
    app::log::info(kOpenWindow, "window size: " + app::toString(kWindowSize));

    // Give the page time to load and render.
    std::this_thread::sleep_for(kPageLoadDelay);

    if (auto r = app::captureWindow(*windowId, output); !r) {
        return fail(kWaitPageLoad, "screenshot failed: " + r.error());
    }
    app::log::info(kWaitPageLoad, "screenshot saved: " + output.string());

    dismissCookieBanner(*windowId, output);

    if (auto r = clickLoginButton(*windowId, output); !r) {
        return fail(kLogin, r.error());
    }

    if (auto r = enterCredentials(*windowId, output, *credentials); !r) {
        return fail(kEnterCredentials, r.error());
    }

    // TakeBet / ReadGameList; SelectGameType is not defined yet.
    if (auto r = readGameList(*windowId, output, *gameListRegion); !r) {
        return fail(kReadGameList, r.error());
    }

    if (auto r = app::openInGimp(output); !r) {
        return fail(kWaitPageLoad, "cannot launch gimp: " + r.error());
    }
    app::log::info(kWaitPageLoad, "opened in gimp");

    return EXIT_SUCCESS;
}
