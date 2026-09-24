#pragma once

#include <expected>
#include <string>
#include <sys/types.h>
#include <vector>

namespace app {

struct ProcessResult {
    int exitCode;
    std::string output;
};

// Runs a program (searched in PATH) without a shell and waits for it, capturing stdout.
std::expected<ProcessResult, std::string> run(const std::vector<std::string>& argv);

// Starts a program in the background, stdout/stderr discarded. Returns its pid.
std::expected<pid_t, std::string> spawnDetached(const std::vector<std::string>& argv);

} // namespace app
