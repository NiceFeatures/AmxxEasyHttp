#pragma once
#include <string>

namespace utils
{
    /// Percent-encodes characters that have special meaning in URLs (RFC 3986).
    /// Used for FTP credentials (user/password) to avoid corrupting the URL.
    std::string PercentEncode(const std::string& input);

    std::string ConstructFtpUrl(const std::string& user, const std::string& password, const std::string& host, const std::string& remote_file);
}