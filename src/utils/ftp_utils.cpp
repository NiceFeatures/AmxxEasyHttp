#include <string>
#include <sstream>
#include <iomanip>
#include <cctype>

namespace utils
{
    std::string PercentEncode(const std::string& input)
    {
        std::ostringstream encoded;
        for (unsigned char c : input)
        {
            // Unreserved characters (RFC 3986) that don't need encoding
            if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
            {
                encoded << c;
            }
            else
            {
                encoded << '%' << std::setw(2) << std::uppercase << std::hex << static_cast<int>(c);
            }
        }
        return encoded.str();
    }

    std::string ConstructFtpUrl(const std::string& user, const std::string& password, const std::string& host, const std::string& remote_file)
    {
        std::ostringstream url;
        url << "ftp://" << PercentEncode(user) << ":" << PercentEncode(password) << "@" << host;

        if (remote_file.empty() || remote_file[0] != '/')
            url << "/";

        url << remote_file;

        return url.str();
    }
}