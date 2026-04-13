#pragma once
#include <atomic>
#include <cstdint>
#include <mutex>

namespace ezhttp
{
    struct RequestControl
    {
        struct Progress
        {
            int32_t download_total{};
            int32_t download_now{};
            int32_t upload_total{};
            int32_t upload_now{};
        };

        void SetProgress(int32_t download_total, int32_t download_now, int32_t upload_total, int32_t upload_now)
        {
            download_total_.store(download_total);
            download_now_.store(download_now);
            upload_total_.store(upload_total);
            upload_now_.store(upload_now);
        }

        [[nodiscard]] Progress GetProgress() const
        {
            return {
                download_total_.load(),
                download_now_.load(),
                upload_total_.load(),
                upload_now_.load()};
        }

        std::atomic_bool completed{false};
        std::atomic_bool forgotten{false};
        std::atomic_bool canceled{false};

        // Mutex for cases where we still need compound safety for user_data or other complex fields
        std::mutex control_mutex;

    private:
        std::atomic<int32_t> download_total_{0};
        std::atomic<int32_t> download_now_{0};
        std::atomic<int32_t> upload_total_{0};
        std::atomic<int32_t> upload_now_{0};
    };
}