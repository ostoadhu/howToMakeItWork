#include "process.hpp"

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>

extern char** environ;

namespace app {

namespace {

std::vector<char*> toArgv(const std::vector<std::string>& args)
{
    std::vector<char*> argv;
    argv.reserve(args.size() + 1);
    for (const auto& a : args) {
        argv.push_back(const_cast<char*>(a.c_str()));
    }
    argv.push_back(nullptr);
    return argv;
}

} // namespace

std::expected<ProcessResult, std::string> run(const std::vector<std::string>& args)
{
    if (args.empty()) {
        return std::unexpected("empty command");
    }

    int pipefd[2];
    if (pipe(pipefd) != 0) {
        return std::unexpected(std::string("pipe: ") + std::strerror(errno));
    }

    posix_spawn_file_actions_t actions;
    posix_spawn_file_actions_init(&actions);
    posix_spawn_file_actions_adddup2(&actions, pipefd[1], STDOUT_FILENO);
    posix_spawn_file_actions_addclose(&actions, pipefd[0]);
    posix_spawn_file_actions_addclose(&actions, pipefd[1]);

    auto argv = toArgv(args);
    pid_t pid{};
    const int rc = posix_spawnp(&pid, argv[0], &actions, nullptr, argv.data(), environ);
    posix_spawn_file_actions_destroy(&actions);
    close(pipefd[1]);

    if (rc != 0) {
        close(pipefd[0]);
        return std::unexpected(args[0] + ": " + std::strerror(rc));
    }

    std::string output;
    char buf[4096];
    for (;;) {
        const ssize_t n = read(pipefd[0], buf, sizeof buf);
        if (n > 0) {
            output.append(buf, static_cast<std::size_t>(n));
        } else if (n < 0 && errno == EINTR) {
            continue;
        } else {
            break;
        }
    }
    close(pipefd[0]);

    int status = 0;
    while (waitpid(pid, &status, 0) < 0 && errno == EINTR) {
    }

    const int exitCode = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
    return ProcessResult{exitCode, std::move(output)};
}

std::expected<pid_t, std::string> spawnDetached(const std::vector<std::string>& args)
{
    if (args.empty()) {
        return std::unexpected("empty command");
    }

    posix_spawn_file_actions_t actions;
    posix_spawn_file_actions_init(&actions);
    posix_spawn_file_actions_addopen(&actions, STDOUT_FILENO, "/dev/null", O_WRONLY, 0);
    posix_spawn_file_actions_addopen(&actions, STDERR_FILENO, "/dev/null", O_WRONLY, 0);

    posix_spawnattr_t attr;
    posix_spawnattr_init(&attr);
    posix_spawnattr_setflags(&attr, POSIX_SPAWN_SETSID);

    auto argv = toArgv(args);
    pid_t pid{};
    const int rc = posix_spawnp(&pid, argv[0], &actions, &attr, argv.data(), environ);
    posix_spawn_file_actions_destroy(&actions);
    posix_spawnattr_destroy(&attr);

    if (rc != 0) {
        return std::unexpected(args[0] + ": " + std::strerror(rc));
    }
    return pid;
}

} // namespace app
