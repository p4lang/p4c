// Copyright (C) 2023 Intel Corporation
// SPDX-FileCopyrightText: 2023 Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0
#include "version.h"

// Return the version string
std::string version_string() {
    std::string version = std::to_string(MAJORVERSION) + "." + std::to_string(MINORVERSION) + ".";
    version += std::to_string(BUILDVERSION) + "." + std::to_string(FIXVERSION);
    return version;
}

unsigned int version_num() {
    return (((MAJORVERSION) << 24) + ((MINORVERSION) << 16) + ((BUILDVERSION) << 8) + (FIXVERSION));
}
