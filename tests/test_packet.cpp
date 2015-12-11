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

#include "bm_sim/packet.h"

TEST(CopyIdGenerator, Test) {
  CopyIdGenerator gen;
  packet_id_t packet_id = 0;
  ASSERT_EQ(0u, gen.get(packet_id));
  ASSERT_EQ(1u, gen.add_one(packet_id));
  ASSERT_EQ(1u, gen.get(packet_id));
  ASSERT_EQ(2u, gen.add_one(packet_id));
  gen.remove_one(packet_id);
  ASSERT_EQ(1u, gen.get(packet_id));
  gen.reset(packet_id);
  ASSERT_EQ(0u, gen.get(packet_id));
}
