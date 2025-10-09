#pragma once
#include <memory>
#include <functional>
#include <atomic>

#ifndef __EMSCRIPTEN__
    #include <mutex>
    #include <thread>
    #include <curl/curl.h>
#else
    #include <emscripten/fetch.h>
#endif

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

        struct DownloadHandler {
            #ifndef __EMSCRIPTEN__
                CURL* eHandle;
            #endif
            std::shared_ptr<DownloadData> data;
        };

        static std::vector<DownloadHandler> currentDownloads;

        #ifndef __EMSCRIPTEN__
            static std::mutex newDownloadsMutex;
            static std::vector<DownloadHandler> newDownloads;
            static std::atomic<bool> shutdownThread;
            static CURLM* mHandle;
            static std::unique_ptr<std::thread> downloadThread;

            static void save_to_string(CURL* curl, std::string* str);
            static void disable_ssl_verification(CURL* curl);
            static size_t write_to_string(void* contents, size_t size, size_t nmemb, void* userp);
            static void add_new_handlers_to_multi();
            static void download_thread_update();
        #else
            static void download_succeeded(emscripten_fetch_t *fetch);
            static void download_failed(emscripten_fetch_t *fetch);
        #endif
};
