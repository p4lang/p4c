/*
Copyright 2020-present Barefoot Networks, Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "exename.h"

#include <array>
#include <cstring>
#include <filesystem>

namespace P4 {

#if defined(__linux__) || defined(__APPLE__) || defined(__FreeBSD__) || defined(__NetBSD__)
#include <unistd.h>

#include <climits>
#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif
#elif defined(_WIN32)
#include <windows.h>
#endif

std::filesystem::path getExecutablePath(const std::filesystem::path &suggestedPath) {
    static std::array<char, PATH_MAX> BUFFER{};

#if defined(__linux__) || defined(__FreeBSD__) || defined(__NetBSD__)
    // Find the path of the executable.  We use a number of techniques that may fail or work on
    // different systems, and take the first working one we find.  Fallback to not overriding the
    // compiled-in installation path.
    std::array<std::string, 4> paths = {
        "/proc/self/exe",         // Linux
        "/proc/curproc/file",     // FreeBSD
        "/proc/curproc/exe",      // NetBSD
        "/proc/self/path/a.out",  // others?
    };

    for (const auto &path : paths) {
        try {
            // std::filesystem::canonical will throw on error if the path is invalid.
            // It will also try to resolve symlinks.
            std::filesystem::path exePath = std::filesystem::canonical(path);
            strncpy(BUFFER.data(), exePath.c_str(), BUFFER.size() - 1);
            BUFFER[BUFFER.size() - 1] = '\0';
            return BUFFER.data();
        } catch (const std::filesystem::filesystem_error &) {
            // Ignore and try the next path.
        }
    }
#elif defined(__APPLE__)
    uint32_t size = static_cast<uint32_t>(BUFFER.size());
    if (_NSGetExecutablePath(BUFFER.data(), &size) == 0) {
        return (BUFFER.data());
    }
#elif defined(_WIN32)
    // TODO: Do we need to support this?
    DWORD size = GetModuleFileNameA(nullptr, BUFFER.data(), static_cast<DWORD>(BUFFER.size()));
    if (size > 0 && size < BUFFER.size()) {
        return (BUFFER.data());
    }
#endif
    // If the above fails, try to convert argv0 to a path.
    if (std::filesystem::exists(suggestedPath)) {
        return suggestedPath;
    }
    return {};
}

const char *exename(const char *argv0) {
    std::filesystem::path argv0Path =
        (argv0 != nullptr) ? std::filesystem::path(argv0) : std::filesystem::path();

    auto path = getExecutablePath(argv0Path);
    if (path.empty()) {
        return nullptr;
    }
    return strdup(path.c_str());
}

}  // namespace P4
