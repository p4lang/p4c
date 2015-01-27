#include <gtest/gtest.h>

#include "data.h"

int pull_test_data() { return 0; }

TEST(Data, ConstructorFromUInt) {
  const Data d(0xaba);
  EXPECT_EQ(d.get_ui(), (unsigned int) 0xaba);
}
