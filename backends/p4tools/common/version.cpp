// SPDX-FileCopyrightText: 2022 The P4 Language Consortium
//
// SPDX-License-Identifier: Apache-2.0

#include "backends/p4tools/common/version.h"

#include <iostream>

void printVersion(const char *binaryName) {
    std::cout << binaryName << std::endl
              << "Version" << " " << P4TOOLS_VERSION_MAJOR << "." << P4TOOLS_VERSION_MINOR << "."
              << P4TOOLS_VERSION_PATCH << P4TOOLS_VERSION_HASH << std::endl;
}
