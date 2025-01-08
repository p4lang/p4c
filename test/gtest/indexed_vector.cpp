#include "ir/indexed_vector.h"

#include <gtest/gtest.h>

#include "ir/ir.h"

namespace P4::Test {

using namespace P4::IR;

using TestVector = IndexedVector<StructField>;

const Type *testType() { return Type_Bits::get(8); }
StructField *testItem(cstring val) { return new StructField(ID(val), testType()); }

TEST(IndexedVector, basics) {
    TestVector vec;
    EXPECT_TRUE(vec.empty());
    EXPECT_EQ(vec.size(), 0u);

    vec.emplace_back(ID("foo"), testType());
    EXPECT_FALSE(vec.empty());
    EXPECT_EQ(vec.size(), 1u);
    EXPECT_EQ(vec[0]->name.name, "foo");

    vec.push_back(testItem("bar"_cs));
    EXPECT_FALSE(vec.empty());
    EXPECT_EQ(vec.size(), 2u);
    EXPECT_EQ(vec[1]->name.name, "bar");

    cstring check = "foo"_cs;
    for (const auto *dec : vec) {
        EXPECT_EQ(dec->name.name, check);
        check = "bar"_cs;
    }

    vec.pop_back();
    EXPECT_FALSE(vec.empty());
    EXPECT_EQ(vec.size(), 1u);
    EXPECT_EQ(vec[0]->name.name, "foo");

    vec.removeByName("foo"_cs);
    EXPECT_TRUE(vec.empty());
    EXPECT_EQ(vec.size(), 0u);
}

TEST(IndexedVector, copy_ctor) {
    TestVector vec;
    TestVector vec2(vec);
    EXPECT_TRUE(vec.empty());
    EXPECT_TRUE(vec2.empty());
    vec2.emplace_back(ID("foo"), testType());
    EXPECT_TRUE(vec.empty());
    EXPECT_FALSE(vec2.empty());

    TestVector vec3(vec2);
    EXPECT_FALSE(vec2.empty());
    EXPECT_FALSE(vec3.empty());
    EXPECT_EQ(vec3.back()->name.name, "foo");
}

TEST(IndexedVector, move_ctor) {
    TestVector vec;
    TestVector vec2(std::move(vec));
    EXPECT_TRUE(vec.empty());
    EXPECT_TRUE(vec2.empty());
    vec2.push_back(testItem("foo"_cs));
    EXPECT_TRUE(vec.empty());
    EXPECT_FALSE(vec2.empty());

    TestVector vec3(std::move(vec2));
    EXPECT_TRUE(vec2.empty());
    EXPECT_FALSE(vec3.empty());
    EXPECT_EQ(vec3.back()->name.name, "foo");
}

TEST(IndexedVector, ilist_ctor) {
    TestVector vec{testItem("foo"_cs), testItem("bar"_cs)};
    EXPECT_EQ(vec.size(), 2u);
    EXPECT_EQ(vec[0]->name.name, "foo");
    EXPECT_EQ(vec[1]->name.name, "bar");
}

TEST(IndexedVector, save_vector_ctor) {
    TestVector vec(safe_vector<const StructField *>{testItem("foo"_cs), testItem("bar"_cs)});
    EXPECT_EQ(vec.size(), 2u);
    EXPECT_EQ(vec[0]->name.name, "foo");
    EXPECT_EQ(vec[1]->name.name, "bar");
}

TEST(IndexedVector, Vector_ctor) {
    TestVector vec(Vector<StructField>{testItem("foo"_cs), testItem("bar"_cs)});
    EXPECT_EQ(vec.size(), 2u);
    EXPECT_EQ(vec[0]->name.name, "foo");
    EXPECT_EQ(vec[1]->name.name, "bar");
}

TEST(IndexedVector, erase) {
    TestVector vec(Vector<StructField>{
        testItem("a"_cs),
        testItem("b"_cs),
        testItem("c"_cs),
        testItem("d"_cs),
        testItem("e"_cs),
        testItem("f"_cs),
        testItem("g"_cs),
    });
    EXPECT_EQ(vec.size(), 7);
    vec.erase(std::next(vec.begin(), 1));  // a c d e f g
    EXPECT_EQ(vec.size(), 6);
    EXPECT_FALSE(vec.getDeclaration("b"));
    vec.validate();

    vec.erase(std::next(vec.begin(), 2), std::next(vec.begin(), 4));  // a c f g
    EXPECT_EQ(vec.size(), 4);
    EXPECT_EQ(vec[0]->name.name, "a");
    EXPECT_EQ(vec[1]->name.name, "c");
    EXPECT_EQ(vec[2]->name.name, "f");
    EXPECT_EQ(vec[3]->name.name, "g");
    EXPECT_FALSE(vec.getDeclaration("d"));
    vec.validate();

    vec.erase(vec.end(), vec.end());
    EXPECT_EQ(vec.size(), 4);
    EXPECT_EQ(vec[0]->name.name, "a");
    EXPECT_EQ(vec[1]->name.name, "c");
    EXPECT_EQ(vec[2]->name.name, "f");
    EXPECT_EQ(vec[3]->name.name, "g");
    EXPECT_FALSE(vec.getDeclaration("e"));
    vec.validate();

    vec.erase(std::next(vec.begin(), 2), vec.end());  // a c
    EXPECT_EQ(vec.size(), 2);
    EXPECT_EQ(vec[0]->name.name, "a");
    EXPECT_EQ(vec[1]->name.name, "c");

    EXPECT_TRUE(vec.getDeclaration("a"));
    EXPECT_TRUE(vec.getDeclaration("c"));
    EXPECT_FALSE(vec.getDeclaration("f"));
    vec.validate();
}

}  // namespace P4::Test
