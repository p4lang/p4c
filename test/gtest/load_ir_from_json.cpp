// Copyright 2019 RT-RK Computer Based Systems.
// SPDX-FileCopyrightText: 2019 RT-RK Computer Based Systems.
//
// SPDX-License-Identifier: Apache-2.0

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <gtest/gtest.h>

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>

#include "helpers.h"
#include "ir/ir.h"
#include "lib/log.h"

using namespace P4;
using namespace std;

namespace P4::Test {

class FromJSONTest : public P4CTest {};

TEST_F(FromJSONTest, load_ir_from_json) {
    int exitCode = system(
        "./p4c-bm2-ss -o outputTO.json test/test_fromJSON.p4 "
        "--toJSON jsonFile.json");
    ASSERT_FALSE(exitCode);
    exitCode = system("./p4c-bm2-ss -o outputFROM.json --fromJSON jsonFile.json");
    ASSERT_FALSE(exitCode);
    exitCode = system(
        "grep -v program outputTO.json > outputTO.json.tmp; "
        "mv outputTO.json.tmp outputTO.json");
    ASSERT_FALSE(exitCode);
    exitCode = system(
        "grep -v program outputFROM.json > outputFROM.json.tmp; "
        "mv outputFROM.json.tmp outputFROM.json");
    ASSERT_FALSE(exitCode);
    exitCode = system("diff outputTO.json outputFROM.json");
    ASSERT_FALSE(exitCode);
    exitCode = system("rm -f outputFROM.json outputTo.json");
    ASSERT_FALSE(exitCode);
}

}  // namespace P4::Test
