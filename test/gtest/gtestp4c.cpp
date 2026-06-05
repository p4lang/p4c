// SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
// Copyright 2013-present Barefoot Networks, Inc.
//
// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

#include "helpers.h"

using namespace P4;

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    AutoCompileContext autoGTestContext(new GTestContext);

    // Initialize the global test environment.
    (void)P4CTestEnvironment::get();

    return RUN_ALL_TESTS();
}
