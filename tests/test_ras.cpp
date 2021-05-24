/* Copyright 2021 VMware, Inc.
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

#include <bm/bm_sim/ras.h>

#include <algorithm>
#include <vector>

using bm::RandAccessUIntSet;

class RandomAccessSetTest : public ::testing::Test { };

TEST_F(RandomAccessSetTest, LargeTestGetNth) {
  RandAccessUIntSet ras;
  std::vector<RandAccessUIntSet::mbr_t> mbrs;
  int size = 128;
  for (auto i = 0; i < size; i++) {
    auto mbr = static_cast<RandAccessUIntSet::mbr_t>(rand() % 65536);
    ras.add(mbr);
    mbrs.push_back(mbr);
  }
  std::sort(mbrs.begin(), mbrs.end());
  size_t nth = 0;
  for (auto i = 0; i < 10000; i++) {
    auto mbr = ras.get_nth(nth);
    ASSERT_EQ(mbrs[nth], mbr);
    nth = (nth + 10) % size;
  }
}
