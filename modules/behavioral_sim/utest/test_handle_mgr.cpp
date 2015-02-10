#include <gtest/gtest.h>

#include "handle_mgr.h"

int pull_test_handle_mgr() { return 0; }

using testing::Types;

template <typename IteratorType>
class SimpleTest : public ::testing::Test {
protected:  
  SimpleTest() {}

  virtual void SetUp() {}

  // virtual void TearDown() {}
};

typedef Types<HandleMgr::iterator, HandleMgr::const_iterator> IteratorTypes;

TYPED_TEST_CASE(SimpleTest, IteratorTypes);

TYPED_TEST(SimpleTest, Iterate) {
  HandleMgr handle_mgr;

  const int N = 32;
  unsigned handles[N];
  
  int rc;
  int i;

  for(i = 0; i < N; i++) {
    rc = handle_mgr.get_handle(&handles[i]);
    ASSERT_EQ(0, rc);
  }

  i = 0;
  for(TypeParam it = handle_mgr.begin(); it != handle_mgr.end(); ++it) {
    ASSERT_EQ(handles[i++], *it);
  }
  ASSERT_EQ(N, i);
}
