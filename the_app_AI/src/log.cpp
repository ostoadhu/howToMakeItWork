#include "log.hpp"

#include <cstdio>
#include <ctime>
#include <fstream>
#include <iostream>
#include <mutex>

namespace app::log {

namespace {

std::mutex mutex;
std::ofstream file;

} // namespace

std::string_view toString(Level level)
{
    switch (level) {
    case Level::Info: return "INFO";
    case Level::Warning: return "WARN";
    case Level::Error: return "ERROR";
    }
    return "?";
}

std::string formatLine(std::chrono::system_clock::time_point time, Level level,
                       std::string_view state, std::string_view message)
{
    const std::time_t seconds = std::chrono::system_clock::to_time_t(time);
    const auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(
                            time.time_since_epoch()).count() % 1000;
    std::tm local{};
    localtime_r(&seconds, &local);

    char stamp[32];
    const std::size_t n = std::strftime(stamp, sizeof stamp, "%Y-%m-%d %H:%M:%S", &local);
    char ms[8];
    std::snprintf(ms, sizeof ms, ".%03d", static_cast<int>(millis < 0 ? millis + 1000 : millis));

    std::string line(stamp, n);
    line += ms;
    line += " [";
    line += toString(level);
    line += "] [";
    line += state;
    line += "] ";
    line += message;
    return line;
}

std::expected<void, std::string> open(const std::filesystem::path& path)
{
    const std::lock_guard lock(mutex);
    if (file.is_open()) {
        file.close();
    }
    file.open(path, std::ios::app);
    if (!file) {
        return std::unexpected("cannot open log file " + path.string());
    }
    return {};
}

void write(Level level, std::string_view state, std::string_view message)
{
    const auto line = formatLine(std::chrono::system_clock::now(), level, state, message);
    const std::lock_guard lock(mutex);
    if (file.is_open()) {
        file << line << '\n' << std::flush;
    }
    (level == Level::Info ? std::cout : std::cerr) << line << '\n';
}

} // namespace app::log
