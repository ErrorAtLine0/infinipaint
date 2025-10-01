#include "FileDownloader.hpp"
#ifndef __EMSCRIPTEN__

#include <curl/multi.h>
#include <iostream>
#include <curl/curl.h>

std::mutex FileDownloader::newDownloadsMutex;
std::vector<FileDownloader::DownloadHandler> FileDownloader::newDownloads;
std::atomic<bool> FileDownloader::shutdownThread;
FileDownloader::MultiHandle FileDownloader::mHandle;
std::vector<FileDownloader::DownloadHandler> FileDownloader::currentDownloads;
std::unique_ptr<std::thread> FileDownloader::downloadThread;

void FileDownloader::init() {
    curl_global_init(CURL_GLOBAL_DEFAULT);
    shutdownThread = false;
}

std::shared_ptr<FileDownloader::DownloadData> FileDownloader::download_data_from_url(std::string_view url) {
    DownloadHandler h;
    auto downloadData = std::make_shared<DownloadData>();
    h.data = downloadData;
    h.data->status = DownloadData::Status::IN_PROGRESS;
    h.eHandle = create_easy_handle();
    curl_easy_setopt(h.eHandle.get(), CURLOPT_URL, url.data());
    disable_ssl_verification(h.eHandle.get());
    save_to_string(h.eHandle.get(), &h.data->str);

    {
        std::scoped_lock newDownloadsLock(newDownloadsMutex);
        newDownloads.emplace_back(std::move(h));
    }

    if(!downloadThread)
        downloadThread = std::make_unique<std::thread>(download_thread_update);

    return downloadData;
}

void FileDownloader::add_new_handlers_to_multi() {
    std::scoped_lock newDownloadsLock(newDownloadsMutex);
    for(auto& handler : newDownloads) {
        curl_multi_add_handle(mHandle.get(), handler.eHandle.get());
        currentDownloads.emplace_back(std::move(handler));
    }
    newDownloads.clear();
}

void FileDownloader::download_thread_update() {
    int stillRunning = 0;

    mHandle = create_multi_handle();
    add_new_handlers_to_multi();

    do {
        if(shutdownThread)
            break;

        CURLMcode mc;
        int numfds;

        mc = curl_multi_perform(mHandle.get(), &stillRunning);

        for(;;) {
            int msgCount;
            CURLMsg* eHandleMsg = curl_multi_info_read(mHandle.get(), &msgCount);
            if(!eHandleMsg)
                break;
            if(eHandleMsg->msg == CURLMSG_DONE) {
                auto it = std::find_if(currentDownloads.begin(), currentDownloads.end(), [eHandleMsg](auto& d) {
                    return eHandleMsg->easy_handle == d.eHandle.get();
                });
                if(it != currentDownloads.end()) {
                    DownloadHandler& download = *it;
                    download.data->status = eHandleMsg->data.result == CURLE_OK ? DownloadData::Status::SUCCESS : DownloadData::Status::FAILURE;
                    currentDownloads.erase(it);
                }
            }
        }

        if(mc == CURLM_OK)
            curl_multi_poll(mHandle.get(), nullptr, 0, 1000, &numfds);
        else {
            std::cout << "curl_multi failed" << std::endl;
            break;
        }
    } while(stillRunning);

    for(auto& download : currentDownloads)
        download.data->status = DownloadData::Status::FAILURE;
    currentDownloads.clear();
}

void FileDownloader::save_to_string(CURL* curl, std::string* str) {
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_string);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, reinterpret_cast<void*>(str));
}

void FileDownloader::disable_ssl_verification(CURL* curl) {
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
}

FileDownloader::EasyHandle FileDownloader::create_easy_handle() {
    auto easyHandle = EasyHandle(curl_easy_init(), curl_easy_cleanup);
    if(!easyHandle)
        throw std::runtime_error("[FileDownloader::create_easy_handle] Failed to create CURL easy handle");
    return easyHandle;
}

FileDownloader::MultiHandle FileDownloader::create_multi_handle() {
    auto multiHandle = EasyHandle(curl_multi_init(), curl_multi_cleanup);
    if(!multiHandle)
        throw std::runtime_error("[FileDownloader::create_multi_handle] Failed to create CURL multi handle");
    return multiHandle;
}

size_t FileDownloader::write_to_string(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t realsize = size * nmemb;
    auto str = reinterpret_cast<std::string*>(userp);
    str->append(reinterpret_cast<const char*>(contents), realsize);
    return realsize;
}

void FileDownloader::cleanup() {
    if(downloadThread && downloadThread->joinable()) {
        shutdownThread = true;
        downloadThread->join();
    }

    curl_global_cleanup();
}

#endif
