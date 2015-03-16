#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gtest/gtest.h>

int main(int argc, char* argv[])
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

