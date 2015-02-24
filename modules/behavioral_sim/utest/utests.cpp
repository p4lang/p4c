#include <gtest/gtest.h>

// implemented in test_data.cpp
int pull_test_data();
// implemented in test_parser.cpp
int pull_test_parser();
// implemented in test_tables.cpp
int pull_test_tables();
// implemented in test_handle_mgr.cpp
int pull_test_handle_mgr();
// implemented in test_queue.cpp
int pull_test_queue();
// implemented in test_p4objects.cpp
int pull_test_p4objects();
// implemented in test_actions.cpp
int pull_test_actions();

// See this link for explanation:
// https://code.google.com/p/googletest/wiki/Primer#Important_note_for_Visual_C++_users
void pull_tests() {
  pull_test_data();
  pull_test_parser();
  pull_test_tables();
  pull_test_handle_mgr();
  pull_test_queue();
  pull_test_p4objects();
  pull_test_actions();
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
