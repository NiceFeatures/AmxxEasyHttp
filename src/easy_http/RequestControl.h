#pragma once

#include <atomic>

namespace ezhttp
{
    struct RequestControl
    {
        std::mutex control_mutex;

        struct Progress
        {
            int32_t download_total;
            int32_t download_now;
            int32_t upload_total;
            int32_t upload_now;
        };

        std::atomic<bool> completed{false};
        std::atomic<bool> forgotten{false};
        // canceled is protected by control_mutex for compound operations
        bool canceled{};
        Progress progress{};
    };
}