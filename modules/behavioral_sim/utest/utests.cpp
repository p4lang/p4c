#include <gtest/gtest.h>

// implemented in test_data.cpp
int pull_test_data();
// implemented in test_parser.cpp
int pull_test_parser();
// implemented in test_packet.cpp
int pull_test_packet();
// implemented in test_tables.cpp
int pull_test_tables();
// implemented in test_handle_mgr.cpp
int pull_test_handle_mgr();

// See this link for explanation:
// https://code.google.com/p/googletest/wiki/Primer#Important_note_for_Visual_C++_users
void pull_tests() {
  pull_test_data();
  pull_test_parser();
  pull_test_packet();
  pull_test_tables();
  pull_test_handle_mgr();
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
