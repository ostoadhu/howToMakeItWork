#pragma once

#include <chrono>
#include <expected>
#include <filesystem>
#include <string>
#include <string_view>

namespace app::log {

enum class Level { Info, Warning, Error };

std::string_view toString(Level level);

// "2026-11-16 12:44:02.123 [INFO] [Login] message" (local time).
std::string formatLine(std::chrono::system_clock::time_point time, Level level,
                       std::string_view state, std::string_view message);

// Opens (appends to) the monitoring log file. Until then, lines go to the terminal only.
std::expected<void, std::string> open(const std::filesystem::path& file);

// Writes one timestamped line to the log file and to the terminal
// (stdout for Info, stderr otherwise). `state` is the state machine state.
void write(Level level, std::string_view state, std::string_view message);

inline void info(std::string_view state, std::string_view message)
{
    write(Level::Info, state, message);
}
inline void warning(std::string_view state, std::string_view message)
{
    write(Level::Warning, state, message);
}
inline void error(std::string_view state, std::string_view message)
{
    write(Level::Error, state, message);
}

} // namespace app::log
