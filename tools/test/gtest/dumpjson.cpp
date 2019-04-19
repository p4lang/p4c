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

#include <iostream>

#include "gtest/gtest.h"
#include "ir/ir.h"
#include "ir/json_loader.h"
#include "ir/visitor.h"

TEST(IR, DumpJSON) {
    auto c = new IR::Constant(2);
    IR::Expression* e1 = new IR::Add(Util::SourceInfo(), c, c);

    std::stringstream ss, ss2;
    JSONGenerator(ss) << e1 << std::endl;
    std::cout << ss.str();

    JSONLoader loader(ss);
    std::cout << loader.json;

    const IR::Node* e2 = nullptr;
    loader >> e2;
    JSONGenerator(std::cout) << e2 << std::endl;
}
