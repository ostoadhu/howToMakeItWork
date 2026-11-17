#include "credentials.hpp"

#include <gtest/gtest.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <unistd.h>

TEST(Credentials, ParsesEmailAndPassword)
{
    const auto c = app::parseCredentials("EMAIL=user@example.com\nPASSWORD=secret\n");
    ASSERT_TRUE(c.has_value()) << c.error();
    EXPECT_EQ(c->email, "user@example.com");
    EXPECT_EQ(c->password, "secret");
}

TEST(Credentials, IgnoresCommentsBlankLinesWhitespaceAndCrlf)
{
    const auto c = app::parseCredentials("# login data\r\n\r\n  EMAIL = user@example.com \r\n"
                                         "PASSWORD=  secret\r\n");
    ASSERT_TRUE(c.has_value()) << c.error();
    EXPECT_EQ(c->email, "user@example.com");
    EXPECT_EQ(c->password, "secret");
}

TEST(Credentials, RemovesSurroundingQuotesButKeepsInnerCharacters)
{
    const auto c = app::parseCredentials("EMAIL=\"user@example.com\"\nPASSWORD='p a=ss#'");
    ASSERT_TRUE(c.has_value()) << c.error();
    EXPECT_EQ(c->email, "user@example.com");
    EXPECT_EQ(c->password, "p a=ss#");
}

TEST(Credentials, MissingValuesAreErrors)
{
    EXPECT_FALSE(app::parseCredentials("PASSWORD=secret").has_value());
    EXPECT_FALSE(app::parseCredentials("EMAIL=user@example.com").has_value());
    EXPECT_FALSE(app::parseCredentials("EMAIL=user@example.com\nPASSWORD=").has_value());
    EXPECT_FALSE(app::parseCredentials("").has_value());
}

TEST(Credentials, MalformedLineAndUnknownKeyAreErrors)
{
    const auto noEq = app::parseCredentials("EMAIL user@example.com\nPASSWORD=secret");
    ASSERT_FALSE(noEq.has_value());
    EXPECT_NE(noEq.error().find("line 1"), std::string::npos);

    const auto unknown = app::parseCredentials("EMAIL=a@b.c\nPASSWORD=secret\nUSER=x");
    ASSERT_FALSE(unknown.has_value());
    EXPECT_NE(unknown.error().find("USER"), std::string::npos);
}

TEST(Credentials, ErrorsNeverContainThePassword)
{
    const auto c = app::parseCredentials("PASSWORD=TopSecret123\nFOO=bar");
    ASSERT_FALSE(c.has_value());
    EXPECT_EQ(c.error().find("TopSecret123"), std::string::npos);
}

TEST(Credentials, LoadsFromFile)
{
    const auto file = std::filesystem::temp_directory_path()
        / ("the_app_AI_credentials_" + std::to_string(::getpid()) + ".env");
    {
        std::ofstream out(file);
        out << "EMAIL=user@example.com\nPASSWORD=secret\n";
    }
    const auto c = app::loadCredentials(file);
    std::filesystem::remove(file);
    ASSERT_TRUE(c.has_value()) << c.error();
    EXPECT_EQ(c->email, "user@example.com");
}

TEST(Credentials, MissingFileIsAnErrorWithHint)
{
    const auto c = app::loadCredentials("/nonexistent/credentials.env");
    ASSERT_FALSE(c.has_value());
    EXPECT_NE(c.error().find("credentials.env.example"), std::string::npos);
}

TEST(Credentials, PathComesFromEnvironmentVariable)
{
    ::unsetenv("THE_APP_AI_CREDENTIALS");
    EXPECT_EQ(app::credentialsPath(), "credentials.env");
    ::setenv("THE_APP_AI_CREDENTIALS", "/etc/the_app_AI/login.env", 1);
    EXPECT_EQ(app::credentialsPath(), "/etc/the_app_AI/login.env");
    ::unsetenv("THE_APP_AI_CREDENTIALS");
}
