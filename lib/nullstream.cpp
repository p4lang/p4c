// SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
// Copyright 2013-present Barefoot Networks, Inc.
//
// SPDX-License-Identifier: Apache-2.0

#include "nullstream.h"

#include <fstream>  // IWYU pragma: keep

#include "lib/error.h"

namespace P4 {

std::unique_ptr<std::ostream> openFile(const std::filesystem::path &name, bool nullOnError) {
    if (name.empty()) {
        if (nullOnError) return std::make_unique<nullstream>();
        ::P4::error(ErrorType::ERR_INVALID, "Empty name for openFile");
        return nullptr;
    }
    auto file = std::make_unique<std::ofstream>(name);
    if (!file->good()) {
        ::P4::error(ErrorType::ERR_IO, "Error writing output to file %1%: %2%", name,
                    strerror(errno));
        if (nullOnError) return std::make_unique<nullstream>();
        return nullptr;
    }
    return file;
}

}  // namespace P4
