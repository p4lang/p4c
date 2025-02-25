/*
Copyright 2013-present Barefoot Networks, Inc.

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

#include <gtest/gtest.h>

#include <iostream>

#include "ir/ir.h"
#include "ir/json_loader.h"
#include "ir/visitor.h"

using namespace P4;
using namespace P4::literals;

TEST(IR, DumpJSON) {
    auto c = new IR::Constant(2);
    IR::Expression *e1 = new IR::Add(Util::SourceInfo(), c, c);

    std::stringstream ss, ss2;
    JSONGenerator(ss).emit(e1);
    std::cout << ss.str();

    JSONLoader loader(ss);
    std::cout << loader.as<JsonData>();

    const IR::Node *e2 = nullptr;
    loader >> e2;
    JSONGenerator(std::cout).emit(e2);
}

TEST(JSON, string_map) {
    std::vector<string_map<std::string>> data, copy;
    data.resize(2);
    data[0]["x"] = "\ttab";
    data[0]["a"] = "\"";
    data[1]["?"] = "question";
    data[1]["-"] = "dash";
    data[1]["\x1c"] = "esc";

    std::stringstream ss;
    JSONGenerator(ss).emit(data);

    // std::cout << ss.str();

    JSONLoader(ss) >> copy;

    EXPECT_EQ(data.size(), copy.size());
    EXPECT_EQ(data[0], copy[0]);
    EXPECT_EQ(data[1], copy[1]);
}

TEST(JSON, std_map) {
    std::map<big_int, bitvec> data, copy;
    data[big_int(1) << 100].setrange(100, 100);
    data[1] = bitvec(1);

    std::stringstream ss;
    JSONGenerator(ss).emit(data);

    // std::cout << ss.str();

    JSONLoader(ss) >> copy;

    EXPECT_EQ(data, copy);
}

TEST(JSON, variant) {
    std::vector<std::variant<int, std::string>> data, copy;
    data.emplace_back(2);
    data.emplace_back(10);
    data.emplace_back("foobar");
    data.emplace_back(10);
    data.emplace_back(1);
    data.emplace_back("x");

    std::stringstream ss;
    JSONGenerator(ss).emit(data);

    // std::cout << ss.str();

    JSONLoader(ss) >> copy;

    EXPECT_EQ(data.size(), copy.size());
    for (size_t i = 0; i < data.size() && i < copy.size(); ++i) {
        EXPECT_EQ(data[i], copy[i]);
    }
}
