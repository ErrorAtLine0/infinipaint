#pragma once
#ifndef __EMSCRIPTEN__

#include <curl/curl.h>
#include <memory>
#include <functional>
#include <atomic>
#include <thread>
#include <mutex>

class FileDownloader {
    public:
        struct DownloadData {
            enum class Status {
                IN_PROGRESS = 0,
                SUCCESS,
                FAILURE
            };
            std::string str;
            std::atomic<Status> status;
        };

        static void init();
        static std::shared_ptr<DownloadData> download_data_from_url(std::string_view url);
        static void cleanup();
    private:
        typedef std::unique_ptr<CURL, std::function<void(CURL*)>> EasyHandle;
        typedef std::unique_ptr<CURLM, std::function<void(CURLM*)>> MultiHandle;

        static void add_new_handlers_to_multi();
        static void download_thread_update();

        struct DownloadHandler {
            EasyHandle eHandle;
            std::shared_ptr<DownloadData> data;
        };

        static std::mutex newDownloadsMutex;
        static std::vector<DownloadHandler> newDownloads;
        static std::atomic<bool> shutdownThread;
        static MultiHandle mHandle;
        static std::vector<DownloadHandler> currentDownloads;
        static std::unique_ptr<std::thread> downloadThread;

        static void save_to_string(CURL* curl, std::string* str);
        static void disable_ssl_verification(CURL* curl);

        static timeval get_timeout(CURLM* mCurl);
        static int wait_if_needed(CURLM* mCurl, timeval& timeout);

        static EasyHandle create_easy_handle();
        static MultiHandle create_multi_handle();

        static size_t write_to_string(void* contents, size_t size, size_t nmemb, void* userp);
};

#endif
