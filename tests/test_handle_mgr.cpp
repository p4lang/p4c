/* Copyright 2013-present Barefoot Networks, Inc.
 * Copyright 2021 VMware, Inc.
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
 * Antonin Bas
 *
 */

#include <gtest/gtest.h>

#include <bm/bm_sim/dynamic_bitset.h>
#include <bm/bm_sim/handle_mgr.h>

using bm::HandleMgr;
using bm::handle_t;

using testing::Types;

template <typename IteratorType>
class HandleMgrIteratorTest : public ::testing::Test {
 protected:
  HandleMgrIteratorTest() {}

  virtual void SetUp() {}

  // virtual void TearDown() {}
};

using IteratorTypes = Types<HandleMgr::iterator, HandleMgr::const_iterator>;

TYPED_TEST_CASE(HandleMgrIteratorTest, IteratorTypes);

TYPED_TEST(HandleMgrIteratorTest, Iterate) {
  HandleMgr handle_mgr;

  const int N = 32;
  handle_t handles[N];

  int rc, i;

  for (i = 0; i < N; i++) {
    rc = handle_mgr.get_handle(&handles[i]);
    ASSERT_EQ(0, rc);
  }

  i = 0;
  for (auto it = handle_mgr.begin(); it != handle_mgr.end(); ++it) {
    ASSERT_EQ(handles[i++], *it);
  }
  ASSERT_EQ(N, i);
}

class HandleMgrTest : public ::testing::Test {
 protected:
  HandleMgrTest() {}

  virtual void SetUp() {}

  // virtual void TearDown() {}
};

TEST_F(HandleMgrTest, LargeTest) {
  HandleMgr handle_mgr;
  const int N = 10000;
  int num_active_handles = 0;
  std::vector<handle_t> handles(N);
  int rc, i;
  for (i = 0; i < N; i++) {
    rc = handle_mgr.get_handle(&handles[i]);
    ASSERT_EQ(0, rc);
    num_active_handles++;
  }
  for (i = 0; i < N; i += 2) {
    rc = handle_mgr.release_handle(handles[i]);
    ASSERT_EQ(0, rc);
    num_active_handles--;
  }
  for (i = 0; i < N; i++) {
    if (i % 2 == 0) {
      ASSERT_FALSE(handle_mgr.valid_handle(handles[i]));
    } else {
      ASSERT_TRUE(handle_mgr.valid_handle(handles[i]));
    }
  }
  i = 0;
  for (auto it = handle_mgr.begin(); it != handle_mgr.end(); ++it) {
    i++;
  }
  ASSERT_EQ(num_active_handles, i);
  for (i = 0; i < N; i += 2) {
    rc = handle_mgr.get_handle(&handles[i]);
    ASSERT_EQ(0, rc);
    num_active_handles++;
  }
  ASSERT_EQ(N, num_active_handles);
  i = 0;
  for (auto it = handle_mgr.begin(); it != handle_mgr.end(); ++it) {
    i++;
  }
  ASSERT_EQ(num_active_handles, i);
}

TEST_F(HandleMgrTest, FirstUnsetUpdateGetHandle) {
  HandleMgr handle_mgr;
  handle_t handle;
  ASSERT_EQ(0, handle_mgr.get_handle(&handle));
  ASSERT_EQ(0, handle_mgr.release_handle(handle));
  ASSERT_EQ(0, handle_mgr.get_handle(&handle));
  ASSERT_EQ(0, handle_mgr.get_handle(&handle));
}

TEST_F(HandleMgrTest, FirstUnsetUpdateSetHandle) {
  const int block_width = std::numeric_limits<bm::DynamicBitset::Block>::digits;
  HandleMgr handle_mgr;
  for (int i = 0; i < block_width; i++) {
    ASSERT_EQ(0, handle_mgr.set_handle(i));
  }
  handle_t handle;
  ASSERT_EQ(0, handle_mgr.get_handle(&handle));
}

TEST_F(HandleMgrTest, SetHandle) {
  HandleMgr handle_mgr;
  ASSERT_EQ(0, handle_mgr.set_handle(0));
  ASSERT_EQ(0, handle_mgr.release_handle(0));
  ASSERT_EQ(0, handle_mgr.set_handle(0));
  ASSERT_EQ(0, handle_mgr.set_handle(1));
}
