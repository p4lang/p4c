#include <gtest/gtest.h>

#include "handle_mgr.h"

int pull_test_handle_mgr() { return 0; }

using testing::Types;

TEST(Handle_mgr, Iterator) {
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
  for(HandleMgr::iterator it = handle_mgr.begin();
      it != handle_mgr.end();
      ++it) {
    ASSERT_EQ(handles[i++], *it);
  }
}

TEST(Handle_mgr, ConstIterator) {
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

  for(HandleMgr::const_iterator it = handle_mgr.begin();
      it != handle_mgr.end();
      ++it) {
    ASSERT_EQ(handles[i++], *it);
  }
}
