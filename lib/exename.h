/*
 * SPDX-FileCopyrightText: 2020 Barefoot Networks, Inc.
 * Copyright 2020-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef LIB_EXENAME_H_
#define LIB_EXENAME_H_

#include <filesystem>

namespace P4 {

/// Return the full path to the binary being executed.
/// @returns @p suggestedPath if unable to determine the path.
/// TODO: With C++20 replace with std::expected?
std::filesystem::path getExecutablePath(const std::filesystem::path &suggestedPath);

/// Return the full path to the binary being executed.
/// @returns an empty path if unable to determine the path.
/// TODO: With C++20 replace with std::expected?
std::filesystem::path getExecutablePath();

/// Attempt to determine the executable name and return a static path to it.
/// Will use argv0 if provided and nothing better can be found
[[deprecated("Use getExecutablePath() instead")]]
const char *exename(const char *argv0 = nullptr);

}  // namespace P4

#endif /* LIB_EXENAME_H_ */
