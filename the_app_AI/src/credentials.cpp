#include "credentials.hpp"

#include <cstdlib>
#include <fstream>
#include <sstream>

namespace app {

namespace {

std::string_view trim(std::string_view s)
{
    const auto first = s.find_first_not_of(" \t\r");
    if (first == std::string_view::npos) {
        return {};
    }
    const auto last = s.find_last_not_of(" \t\r");
    return s.substr(first, last - first + 1);
}

std::string_view unquote(std::string_view s)
{
    if (s.size() >= 2 && (s.front() == '"' || s.front() == '\'') && s.back() == s.front()) {
        return s.substr(1, s.size() - 2);
    }
    return s;
}

} // namespace

std::expected<Credentials, std::string> parseCredentials(std::string_view text)
{
    Credentials c;
    std::size_t start = 0;
    int lineNo = 0;
    while (start <= text.size()) {
        const auto end = text.find('\n', start);
        const auto line = trim(text.substr(start, end == std::string_view::npos ? std::string_view::npos
                                                                                   : end - start));
        ++lineNo;
        start = end == std::string_view::npos ? text.size() + 1 : end + 1;
        if (line.empty() || line.front() == '#') {
            continue;
        }
        const auto eq = line.find('=');
        if (eq == std::string_view::npos) {
            return std::unexpected("line " + std::to_string(lineNo) + ": expected KEY=VALUE");
        }
        const auto key = trim(line.substr(0, eq));
        const auto value = unquote(trim(line.substr(eq + 1)));
        if (key == "EMAIL") {
            c.email = value;
        } else if (key == "PASSWORD") {
            c.password = value;
        } else {
            return std::unexpected("line " + std::to_string(lineNo) + ": unknown key '"
                                   + std::string(key) + "'");
        }
    }
    if (c.email.empty()) {
        return std::unexpected("EMAIL is missing or empty");
    }
    if (c.password.empty()) {
        return std::unexpected("PASSWORD is missing or empty");
    }
    return c;
}

std::expected<Credentials, std::string> loadCredentials(const std::filesystem::path& file)
{
    std::ifstream in(file);
    if (!in) {
        return std::unexpected("cannot read " + file.string()
                               + " (copy credentials.env.example to credentials.env)");
    }
    std::stringstream content;
    content << in.rdbuf();
    auto c = parseCredentials(content.str());
    if (!c) {
        return std::unexpected(file.string() + ": " + c.error());
    }
    return c;
}

std::filesystem::path credentialsPath()
{
    if (const char* env = std::getenv("THE_APP_AI_CREDENTIALS"); env && *env) {
        return env;
    }
    return "credentials.env";
}

} // namespace app
