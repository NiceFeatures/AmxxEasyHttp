#include "UrlUtils.h"

namespace ezhttp
{
    thread_local CURLU* UrlUtils::curl_url_ = nullptr;

    std::string UrlUtils::GetHostByUrl(const std::string& url)
    {
        InitializeIfNeeded();

        CURLUcode rc;
        char* host = nullptr;

        rc = curl_url_set(curl_url_, CURLUPART_URL, url.c_str(), 0);
        if (rc != CURLUE_OK)
            return "";

        rc = curl_url_get(curl_url_, CURLUPART_HOST, &host, 0);
        if (rc != CURLUE_OK)
            return "";

        std::string result(host);
        curl_free(host);
        return result;
    }

    void UrlUtils::InitializeIfNeeded()
    {
        if (curl_url_ != nullptr)
            return;

        curl_url_ = curl_url();
    }

    void UrlUtils::Cleanup()
    {
        if (curl_url_ != nullptr)
        {
            curl_url_cleanup(curl_url_);
            curl_url_ = nullptr;
        }
    }
}
