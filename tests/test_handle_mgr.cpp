/* Copyright 2013-present Barefoot Networks, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Antonin Bas (antonin@barefootnetworks.com)
 *
 */

#include <gtest/gtest.h>

#include <bm/bm_sim/handle_mgr.h>

using bm::HandleMgr;
using bm::handle_t;

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
  handle_t handles[N];
  
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
