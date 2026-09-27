// SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
// Copyright 2013-present Barefoot Networks, Inc.
//
// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

#include <iostream>

#include "ir/ir.h"
#include "ir/json_loader.h"
#include "ir/visitor.h"

using namespace P4;
using namespace P4::literals;

TEST(IR, DumpJSON) {
    auto c = new IR::Constant(2);
    IR::MutablePtr<IR::Expression> e1 = new IR::Add(Util::SourceInfo(), c, c);

    std::stringstream ss, ss2;
    JSONGenerator(ss).emit(e1);
    std::cout << ss.str();

    JSONLoader loader(ss);
    std::cout << loader.as<JsonData>();

    IR::Ptr<IR::Node> e2 = nullptr;
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

TEST(JSON, SharedAnnotationConstant) {
    auto value = std::make_shared<UnparsedConstant>(UnparsedConstant{"8w42"_cs, 0, 10, true});
    std::weak_ptr<UnparsedConstant> lifetime = value;
    std::stringstream encoded;
    {
        IR::Ptr<IR::AnnotationToken> token = new IR::AnnotationToken(1, "8w42"_cs, value);
        value.reset();
        JSONGenerator(encoded).emit(token);
    }
#if !HAVE_LIBGC
    EXPECT_TRUE(lifetime.expired());
#endif
    IR::Ptr<IR::AnnotationToken> decoded;
    JSONLoader loader(encoded);
    loader >> decoded;
    ASSERT_NE(decoded, nullptr);
    ASSERT_NE(decoded->constInfo, nullptr);
    EXPECT_EQ(decoded->constInfo->text, "8w42");
    EXPECT_EQ(decoded->constInfo->base, 10U);
    EXPECT_TRUE(decoded->constInfo->hasWidth);
}

TEST(JSON, DecodedGraphOutlivesLoader) {
    std::stringstream encoded;
    {
        IR::Ptr<IR::Constant> value = new IR::Constant(42);
        IR::Ptr<IR::Add> expression = new IR::Add(value, value);
        JSONGenerator(encoded).emit(expression);
    }
    IR::Ptr<IR::Add> decoded;
    {
        JSONLoader loader(encoded);
        loader >> decoded;
    }
    ASSERT_NE(decoded, nullptr);
    EXPECT_EQ(decoded->left, decoded->right);
    EXPECT_EQ(decoded->left->checkedTo<IR::Constant>()->value, 42);
}
