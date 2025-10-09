#ifdef __EMSCRIPTEN__
#include "FileDownloader.hpp"
#include <cstdio>
#include <iostream>

std::vector<FileDownloader::DownloadHandler> FileDownloader::currentDownloads;

void FileDownloader::download_progress(emscripten_fetch_t *fetch) {
    DownloadData* downData = static_cast<DownloadData*>(fetch->userData);
    if(fetch->totalBytes == 0)
        downData->progress = 0.0f;
    else
        downData->progress = static_cast<float>(fetch->dataOffset) / static_cast<float>(fetch->totalBytes);
}

void FileDownloader::download_succeeded(emscripten_fetch_t *fetch) {
    DownloadData* downData = static_cast<DownloadData*>(fetch->userData);
    downData->str = std::string(fetch->data, fetch->numBytes);
    downData->status = DownloadData::Status::SUCCESS;
    std::erase_if(currentDownloads, [fetch](DownloadHandler& d) {
        return (DownloadData*)fetch->userData == d.data.get();
    });
    emscripten_fetch_close(fetch);
}

void FileDownloader::download_failed(emscripten_fetch_t *fetch) {
    DownloadData* downData = static_cast<DownloadData*>(fetch->userData);
    downData->status = DownloadData::Status::FAILURE;
    std::erase_if(currentDownloads, [fetch](DownloadHandler& d) {
        return (DownloadData*)fetch->userData == d.data.get();
    });
    emscripten_fetch_close(fetch);
}

void FileDownloader::init() {
}

std::shared_ptr<FileDownloader::DownloadData> FileDownloader::download_data_from_url(std::string_view url) {
    DownloadHandler& newDownload = currentDownloads.emplace_back();
    newDownload.data = std::make_shared<DownloadData>();

    emscripten_fetch_attr_t attr;
    emscripten_fetch_attr_init(&attr);
    std::strcpy(attr.requestMethod, "GET");
    attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
    attr.onsuccess = download_succeeded;
    attr.onprogress = download_progress;
    attr.onerror = download_failed;
    attr.userData = static_cast<void*>(newDownload.data.get());
    emscripten_fetch(&attr, url.data());

    return newDownload.data;
}

void FileDownloader::cleanup() {
}

#endif
