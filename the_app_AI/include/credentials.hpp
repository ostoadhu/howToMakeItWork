#pragma once

#include <expected>
#include <filesystem>
#include <string>
#include <string_view>

namespace app {

struct Credentials {
    std::string email;
    std::string password;
};

// Parses KEY=VALUE lines (EMAIL, PASSWORD). '#' starts a comment line;
// whitespace around keys/values and surrounding quotes are removed.
std::expected<Credentials, std::string> parseCredentials(std::string_view text);

// Reads the credentials file. It must never be committed (see .gitignore).
std::expected<Credentials, std::string> loadCredentials(const std::filesystem::path& file);

// $THE_APP_AI_CREDENTIALS if set, otherwise ./credentials.env
std::filesystem::path credentialsPath();

} // namespace app
