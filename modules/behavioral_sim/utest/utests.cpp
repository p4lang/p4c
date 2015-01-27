#include <gtest/gtest.h>

int pull_test_data();

void pull_tests() {
  pull_test_data();
}

#ifdef __cplusplus
extern "C" {
#endif

int run_all_gtests(int argc, char **argv) {
  pull_tests();

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

#ifdef __cplusplus
}
#endif
