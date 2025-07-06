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
    newCWD += "/Contents/Resources";

    std::filesystem::current_path(std::filesystem::path(newCWD));
#elif _WIN32
	char buffer[4096];
	GetModuleFileName(NULL, buffer, 4096);
	std::filesystem::path exePath(buffer);
	std::filesystem::current_path(exePath.parent_path());
#elif __linux__
    std::filesystem::path exePath = std::filesystem::canonical("/proc/self/exe");
    std::filesystem::current_path(exePath.parent_path());
#endif
}
