#include "SwitchCWD.hpp"
#include <iostream>
#include <filesystem>

#ifdef __APPLE__
#include <CoreFoundation/CFBundle.h>
#elif _WIN32
#include <Shlwapi.h>
#endif



void switch_cwd() {
#ifdef __APPLE__
    CFBundleRef mainBundle = CFBundleGetMainBundle();
    CFURLRef resourcesURL = CFBundleCopyBundleURL(mainBundle);
    CFStringRef str = CFURLCopyFileSystemPath(resourcesURL, kCFURLPOSIXPathStyle);
    CFRelease(resourcesURL);
    char path[4096];

    CFStringGetCString(str, path, 4096, kCFStringEncodingUTF8);
    CFRelease(str);
    std::string newCWD(path);
#ifdef MACOS_MAKE_BUNDLE
    newCWD += "/Contents/Resources";
#endif

    std::filesystem::current_path(std::filesystem::path(newCWD));
#elif _WIN32
    // https://stackoverflow.com/a/33613252 but UTF-8
    constexpr int MAX_PATH_STEP = 2048;
    std::string pathStr;
    DWORD copied = 0;
    do {
        pathStr.resize(pathStr.size() + MAX_PATH_STEP);
        copied = GetModuleFileName(NULL, pathStr.data(), pathStr.size());
    } while(copied >= pathStr.size());
    pathStr.resize(copied);
    
	std::filesystem::path exePath(pathStr);
	std::filesystem::current_path(exePath.parent_path());
#elif __linux__
    std::filesystem::path exePath = std::filesystem::canonical("/proc/self/exe");
    std::filesystem::current_path(exePath.parent_path());
#endif
}
