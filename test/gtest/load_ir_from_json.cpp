/*
Copyright 2019 RT-RK Computer Based Systems.

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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>

#include "gtest/gtest.h"
#include "helpers.h"
#include "ir/ir.h"
#include "lib/log.h"

using namespace P4;
using namespace std;

namespace Test {

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

}  // namespace Test
