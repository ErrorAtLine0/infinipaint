#include "FileDownloader.hpp"

#ifndef __EMSCRIPTEN__

#include <curl/multi.h>
#include <iostream>
#include <curl/curl.h>
#include <algorithm>

std::mutex FileDownloader::newDownloadsMutex;
std::vector<FileDownloader::DownloadHandler> FileDownloader::newDownloads;
std::atomic<bool> FileDownloader::shutdownThread;
CURLM* FileDownloader::mHandle;
std::vector<FileDownloader::DownloadHandler> FileDownloader::currentDownloads;
std::unique_ptr<std::thread> FileDownloader::downloadThread;

void FileDownloader::init() {
    curl_global_init(CURL_GLOBAL_DEFAULT);
    shutdownThread = false;
}

std::shared_ptr<FileDownloader::DownloadData> FileDownloader::download_data_from_url(const std::string& url) {
    DownloadHandler h;
    auto downloadData = std::make_shared<DownloadData>();
    h.data = downloadData;
    h.eHandle = curl_easy_init();
    downloadData->fileName = get_potential_filename_from_url(url);

    curl_easy_setopt(h.eHandle, CURLOPT_URL, url.data());
    curl_easy_setopt(h.eHandle, CURLOPT_NOSIGNAL, 1);
    curl_easy_setopt(h.eHandle, CURLOPT_FOLLOWLOCATION, CURLFOLLOW_ALL);
    curl_easy_setopt(h.eHandle, CURLOPT_USERAGENT, "InfiniPaint/0.2.0");
    curl_easy_setopt(h.eHandle, CURLOPT_NOPROGRESS, 0);
    curl_easy_setopt(h.eHandle, CURLOPT_XFERINFODATA, h.data.get());
    curl_easy_setopt(h.eHandle, CURLOPT_XFERINFOFUNCTION, download_progress_callback);
    curl_easy_setopt(h.eHandle, CURLOPT_HEADERFUNCTION, header_callback);
    curl_easy_setopt(h.eHandle, CURLOPT_HEADERDATA, h.data.get());

    disable_ssl_verification(h.eHandle);
    save_to_string(h.eHandle, &h.data->str);

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
        curl_multi_add_handle(mHandle, handler.eHandle);
        currentDownloads.emplace_back(std::move(handler));
    }
    newDownloads.clear();
}

void FileDownloader::download_thread_update() {
    int stillRunning = 0;

    mHandle = curl_multi_init();

    for(;;) {
        if(shutdownThread)
            break;

        CURLMcode mc;
        int numfds;

        add_new_handlers_to_multi();
        mc = curl_multi_perform(mHandle, &stillRunning);

        for(;;) {
            int msgCount;
            CURLMsg* eHandleMsg = curl_multi_info_read(mHandle, &msgCount);
            if(!eHandleMsg)
                break;
            if(eHandleMsg->msg == CURLMSG_DONE) {
                auto it = std::find_if(currentDownloads.begin(), currentDownloads.end(), [eHandleMsg](auto& d) {
                    return eHandleMsg->easy_handle == d.eHandle;
                });
                if(it != currentDownloads.end()) {
                    DownloadHandler& download = *it;
                    download.data->status = eHandleMsg->data.result == CURLE_OK ? DownloadData::Status::SUCCESS : DownloadData::Status::FAILURE;
                    curl_multi_remove_handle(mHandle, download.eHandle);
                    curl_easy_cleanup(download.eHandle);
                    currentDownloads.erase(it);
                }
            }
        }

        if(mc == CURLM_OK)
            curl_multi_poll(mHandle, nullptr, 0, 1000, &numfds);
        else {
            std::cout << "curl_multi failed" << std::endl;
            break;
        }
    }

    for(auto& download : currentDownloads) {
        download.data->status = DownloadData::Status::FAILURE;
        curl_multi_remove_handle(mHandle, download.eHandle);
        curl_easy_cleanup(download.eHandle);
    }

    currentDownloads.clear();
    curl_multi_cleanup(mHandle);
}

void FileDownloader::save_to_string(CURL* curl, std::string* str) {
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_string);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, reinterpret_cast<void*>(str));
}

void FileDownloader::disable_ssl_verification(CURL* curl) {
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
}

size_t FileDownloader::write_to_string(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t realsize = size * nmemb;
    auto str = reinterpret_cast<std::string*>(userp);
    str->append(reinterpret_cast<const char*>(contents), realsize);
    return realsize;
}

int FileDownloader::download_progress_callback(void *clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow) {
    DownloadData* downData = static_cast<DownloadData*>(clientp);
    if(dltotal == 0)
        downData->progress = 0.0f;
    else
        downData->progress = static_cast<float>(dlnow) / static_cast<float>(dltotal);
    return 0;
}

size_t FileDownloader::header_callback(char* buffer, size_t size, size_t nitems, void* userData) {
    DownloadData* downData = static_cast<DownloadData*>(userData);
    std::string header(buffer, nitems);
    std::string newFilename = parse_http_headers_for_filename(header);
    if(!newFilename.empty())
        downData->fileName = newFilename;

    return nitems;
}

void FileDownloader::cleanup() {
    if(downloadThread && downloadThread->joinable()) {
        shutdownThread = true;
        downloadThread->join();
    }
    downloadThread = nullptr;

    curl_global_cleanup();
}

#endif

std::string FileDownloader::parse_http_headers_for_filename(std::string header) {
    std::string headerLower = header;
    std::transform(headerLower.begin(), headerLower.end(), headerLower.begin(), ::tolower);
    std::string contentDispositionHeader = "content-disposition:";
    size_t contentDispositionLoc = headerLower.find(contentDispositionHeader);
    if(contentDispositionLoc < headerLower.size()) {
        size_t newlineHeaderLoc = std::min<size_t>(headerLower.find('\n', contentDispositionLoc), headerLower.size());

        std::string filenameHeader = "filename=";
        size_t filenameLoc = headerLower.find(filenameHeader, contentDispositionLoc);
        if(filenameLoc < newlineHeaderLoc) {
            header = header.substr(filenameLoc + filenameHeader.size());
            header = header.substr(0, header.find(';'));
            if(header.size() >= 2 && header.front() == '"' && header.back() == '"') {
                header.erase(0, 1);
                header.pop_back();
            }
            return header;
        }
    }

    return "";
}

std::string FileDownloader::get_potential_filename_from_url(const std::string& url) {
    size_t fileNameStart = url.find_last_of('/');
    if(fileNameStart >= (url.size() - 1))
        return "";
    std::string fileName = url.substr(fileNameStart + 1);
    return fileName.substr(0, fileName.find('?'));
}
