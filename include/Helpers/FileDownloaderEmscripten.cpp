/*  
 * InfiniPaint
 * Copyright (C) 2025-2026 Yousef Khadadeh
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifdef __EMSCRIPTEN__
#include "FileDownloader.hpp"
#include <cstdio>
#include <iostream>
#include <vector>

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

    size_t headerLength = emscripten_fetch_get_response_headers_length(fetch);
    if(headerLength != 0) {
        std::string header(headerLength + 1, ' ');
        emscripten_fetch_get_response_headers(fetch, header.data(), header.size() + 1);
        if(!header.empty())
            header.pop_back(); // Remove additional null terminator
        std::string newFilename = parse_http_headers_for_filename(header);
        if(!newFilename.empty())
            downData->fileName = newFilename;
    }

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

std::shared_ptr<FileDownloader::DownloadData> FileDownloader::download_data_from_url(const std::string& url) {
    DownloadHandler& newDownload = currentDownloads.emplace_back();
    newDownload.data = std::make_shared<DownloadData>();
    newDownload.data->fileName = get_potential_filename_from_url(url);

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
