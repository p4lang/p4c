// SPDX-FileCopyrightText: 2020 Barefoot Networks, Inc.
// Copyright 2020-present Barefoot Networks, Inc.
//
// SPDX-License-Identifier: Apache-2.0

#include "exename.h"

#include <array>
#include <climits>
#include <cstring>
#include <filesystem>
#include <system_error>

#ifdef __APPLE__
#include <unistd.h>

#include <mach-o/dyld.h>
#elif defined(_WIN32)
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace P4 {

std::filesystem::path getExecutablePath() {
#if defined(__APPLE__)
    std::array<char, PATH_MAX> buffer{};
    uint32_t size = static_cast<uint32_t>(buffer.size());
    if (_NSGetExecutablePath(buffer.data(), &size) == 0) {
        return buffer.data();
    }
#elif defined(_WIN32)
    std::array<char, PATH_MAX> buffer{};
    // TODO: Do we need to support this?
    DWORD size = GetModuleFileNameA(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (size > 0 && size < buffer.size()) {
        return buffer.data();
    }
#else
    // Find the path of the executable.  We use a number of techniques that may fail or work on
    // different systems, and take the first working one we find.  Fallback to not overriding the
    // compiled-in installation path.
    std::array<std::string, 4> paths = {
        "/proc/self/exe",         // Linux
        "/proc/curproc/file",     // FreeBSD
        "/proc/curproc/exe",      // NetBSD
        "/proc/self/path/a.out",  // Solaris
    };

    for (const auto &path : paths) {
        // std::filesystem::canonical will fail if the path is invalid.
        // It will also try to resolve symlinks.
        std::error_code errorCode;
        auto canonicalPath = std::filesystem::canonical(path, errorCode);
        // Return the path if no error occurred.
        if (!errorCode) {
            return canonicalPath;
        }
    }
#endif
    return std::filesystem::path();
}

std::filesystem::path getExecutablePath(const std::filesystem::path &suggestedPath) {
    auto path = getExecutablePath();
    if (!path.empty()) {
        return path;
    }
    // If the above fails, try to convert suggestedPath to a path.
    std::error_code errorCode;
    auto canonicalPath = std::filesystem::canonical(suggestedPath, errorCode);
    return errorCode ? std::filesystem::path() : canonicalPath;
}

const char *exename(const char *argv0) {
    std::filesystem::path argv0Path =
        (argv0 != nullptr) ? std::filesystem::path(argv0) : std::filesystem::path();

    auto path = getExecutablePath(argv0Path);
    if (path.empty()) {
        return nullptr;
    }
    // TODO: There is a potential leak here.
    return strdup(path.c_str());
}

}  // namespace P4
