#include "process.hpp"

#include <gtest/gtest.h>

#include <csignal>
#include <sys/wait.h>

TEST(ProcessRun, CapturesStdout)
{
    const auto res = app::run({"echo", "hello world"});
    ASSERT_TRUE(res.has_value()) << res.error();
    EXPECT_EQ(res->exitCode, 0);
    EXPECT_EQ(res->output, "hello world\n");
}

TEST(ProcessRun, ReportsNonZeroExitCode)
{
    const auto res = app::run({"sh", "-c", "exit 3"});
    ASSERT_TRUE(res.has_value()) << res.error();
    EXPECT_EQ(res->exitCode, 3);
}

TEST(ProcessRun, ArgumentsAreNotInterpretedByAShell)
{
    // If a shell were involved, `$HOME` would be expanded and `;` would split commands.
    const auto res = app::run({"echo", "$HOME; echo injected"});
    ASSERT_TRUE(res.has_value()) << res.error();
    EXPECT_EQ(res->output, "$HOME; echo injected\n");
}

TEST(ProcessRun, EmptyCommandIsAnError)
{
    const auto res = app::run({});
    ASSERT_FALSE(res.has_value());
    EXPECT_EQ(res.error(), "empty command");
}

TEST(ProcessRun, UnknownProgramIsAnError)
{
    const auto res = app::run({"this-program-does-not-exist-42"});
    ASSERT_FALSE(res.has_value());
    EXPECT_NE(res.error().find("this-program-does-not-exist-42"), std::string::npos);
}

TEST(ProcessSpawnDetached, ReturnsPidOfRunningChild)
{
    const auto pid = app::spawnDetached({"sleep", "5"});
    ASSERT_TRUE(pid.has_value()) << pid.error();
    EXPECT_GT(*pid, 0);
    EXPECT_EQ(kill(*pid, 0), 0) << "child should be alive";

    kill(*pid, SIGKILL);
    waitpid(*pid, nullptr, 0);
}

TEST(ProcessSpawnDetached, EmptyCommandIsAnError)
{
    const auto pid = app::spawnDetached({});
    ASSERT_FALSE(pid.has_value());
    EXPECT_EQ(pid.error(), "empty command");
}
