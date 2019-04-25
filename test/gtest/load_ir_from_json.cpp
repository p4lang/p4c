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

#include <unistd.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>
#include <thread>
#include <cstdlib>

#include "gtest/gtest.h"
#include "ir/ir.h"
#include "helpers.h"
#include "lib/log.h"

using namespace P4;
using namespace std;

namespace Test {

class FromJSONTest : public P4CTest { };

TEST_F(FromJSONTest, load_ir_from_json) {
    #if defined(__linux__)
    pid_t pid = vfork();
    if (pid == -1) {
        ::error("fork() failed");
    } else if (pid == 0) {
        execlp("p4c-bm2-ss", "p4c-bm2-ss", "-o", "outputTO.json", "../test/test_fromJSON.p4", "--toJSON", "jsonFile.json", (char *)NULL);    //NOLINT
    }
    int child_status;
    int child_pid = wait(&child_status);
    pid_t pid2 = vfork();
    if (pid2 == -1) {
        ::error("fork() failed");
    } else if (pid2 == 0) {
        execlp("p4c-bm2-ss", "p4c-bm2-ss", "-o", "outputFROM.json", "--fromJSON", "jsonFile.json", (char *)NULL);   //NOLINT
    }
    int child_status2;
    int child_pid2 = wait(&child_status2);

    fstream file1("outputTO.json"), file2("outputFROM.json");
    char string1[256], string2[256];
    int j; j = 0;
    while (!file1.eof()) {
        file1.getline(string1, 256);
        file2.getline(string2, 256);
        j++;
        if (strcmp(string1, string2) != 0) {
            ASSERT_FALSE(strstr(string1, "program") == nullptr \
                || strstr(string2, "program") == nullptr);
        }
    }
    #endif
    ASSERT_TRUE(true);
}

}  // namespace Test
