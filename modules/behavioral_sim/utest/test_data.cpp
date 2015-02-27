#include <gtest/gtest.h>

#include "behavioral_sim/data.h"

int pull_test_data() { return 0; }

TEST(Data, ConstructorFromUInt) {
  const Data d(0xaba);
  EXPECT_EQ(d.get_ui(), (unsigned) 0xaba);
}

TEST(Data, EqualityOp) {
  const Data d1(0xaba);
  const Data d2(0xaba);
  ASSERT_TRUE(d1 == d2);
}

TEST(Data, InEqualityOp) {
  const Data d1(0xaba);
  const Data d2(0xab0);
  ASSERT_TRUE(d1 != d2);
}

TEST(Data, CopyOp) {
  const Data d1(0xaba);
  Data d2;
  d2 = d1;
  EXPECT_EQ(d1, d2);
  d2.set(0);
  EXPECT_EQ(d1.get_ui(), (unsigned) 0xaba);
}

TEST(Data, AddDestIsSrc) {
  Data d1(11);
  Data d2(22);
  d1.add(d1, d2);
  EXPECT_EQ(d1.get_ui(), (unsigned) 33);
}

TEST(Data, Add) {
  Data d1(11);
  Data d2(22);
  Data d3;
  d3.add(d1, d2);
  EXPECT_EQ(d3.get_ui(), (unsigned) 33);
}
