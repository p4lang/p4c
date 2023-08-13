#include "ir/indexed_vector.h"

#include <string>
#include <vector>

#include "gtest/gtest.h"
#include "ir/id.h"
#include "ir/ir.h"

namespace Test {

using namespace IR;

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

    vec.push_back(testItem("bar"));
    EXPECT_FALSE(vec.empty());
    EXPECT_EQ(vec.size(), 2u);
    EXPECT_EQ(vec[1]->name.name, "bar");

    cstring check = "foo";
    for (auto *dec : vec) {
        EXPECT_EQ(dec->name.name, check);
        check = "bar";
    }

    vec.pop_back();
    EXPECT_FALSE(vec.empty());
    EXPECT_EQ(vec.size(), 1u);
    EXPECT_EQ(vec[0]->name.name, "foo");

    vec.removeByName("foo");
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
    vec2.push_back(testItem("foo"));
    EXPECT_TRUE(vec.empty());
    EXPECT_FALSE(vec2.empty());

    TestVector vec3(std::move(vec2));
    EXPECT_TRUE(vec2.empty());
    EXPECT_FALSE(vec3.empty());
    EXPECT_EQ(vec3.back()->name.name, "foo");
}

TEST(IndexedVector, ilist_ctor) {
    TestVector vec{testItem("foo"), testItem("bar")};
    EXPECT_EQ(vec.size(), 2u);
    EXPECT_EQ(vec[0]->name.name, "foo");
    EXPECT_EQ(vec[1]->name.name, "bar");
}

TEST(IndexedVector, save_vector_ctor) {
    TestVector vec(safe_vector<const StructField *>{testItem("foo"), testItem("bar")});
    EXPECT_EQ(vec.size(), 2u);
    EXPECT_EQ(vec[0]->name.name, "foo");
    EXPECT_EQ(vec[1]->name.name, "bar");
}

TEST(IndexedVector, Vector_ctor) {
    TestVector vec(Vector<StructField>{testItem("foo"), testItem("bar")});
    EXPECT_EQ(vec.size(), 2u);
    EXPECT_EQ(vec[0]->name.name, "foo");
    EXPECT_EQ(vec[1]->name.name, "bar");
}

}  // namespace Test
