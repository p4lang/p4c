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

#include <vector>
#include <memory>

#include <bm/bm_sim/packet.h>

using namespace bm;

TEST(CopyIdGenerator, Test) {
  CopyIdGenerator gen;
  packet_id_t packet_id = 0;
  ASSERT_EQ(0u, gen.get(packet_id));
  ASSERT_EQ(1u, gen.add_one(packet_id));
  ASSERT_EQ(1u, gen.get(packet_id));
  ASSERT_EQ(2u, gen.add_one(packet_id));
  gen.remove_one(packet_id);
  ASSERT_EQ(2u, gen.get(packet_id));
  gen.reset(packet_id);
  ASSERT_EQ(0u, gen.get(packet_id));
}

class PHVSourceTest : public PHVSourceIface {
 public:
  explicit PHVSourceTest(size_t size)
      : phv_factories(size, nullptr), created(size, 0u), destroyed(size, 0u) { }

  size_t get_created(size_t cxt) {
    return created.at(cxt);
  }

  size_t get_destroyed(size_t cxt) {
    return destroyed.at(cxt);
  }

 private:
  std::unique_ptr<PHV> get_(size_t cxt) override {
    assert(phv_factories[cxt]);
    ++count;
    ++created.at(cxt);
    return phv_factories[cxt]->create();
  }

  void release_(size_t cxt, std::unique_ptr<PHV> phv) override {
    // let the PHV be destroyed
    (void) cxt; (void) phv;
    --count;
    ++destroyed.at(cxt);
  }

  void set_phv_factory_(size_t cxt, const PHVFactory *factory) override {
    phv_factories.at(cxt) = factory;
  }

  size_t phvs_in_use_(size_t cxt) override {
    return count;
  }

  std::vector<const PHVFactory *> phv_factories;
  size_t count{0};
  std::vector<size_t> created;
  std::vector<size_t> destroyed;
};

// Google Test fixture for Packet tests
class PacketTest : public ::testing::Test {
 protected:
  PHVFactory phv_factory;

  std::unique_ptr<PHVSourceTest> phv_source{nullptr};

  PacketTest()
      : phv_source(new PHVSourceTest(2)) { }

  virtual void SetUp() {
    phv_source->set_phv_factory(0, &phv_factory);
    phv_source->set_phv_factory(1, &phv_factory);
  }

  // virtual void TearDown() { }

  Packet get_packet(size_t cxt, packet_id_t id = 0) {
    // dummy packet, never parsed
    return Packet::make_new(cxt, 0, id, 0, 0, PacketBuffer(), phv_source.get());
  }
};

TEST_F(PacketTest, Packet) {
  const size_t first_cxt = 0;
  const size_t other_cxt = 1;
  ASSERT_EQ(0u, phv_source->get_created(first_cxt));
  ASSERT_EQ(0u, phv_source->get_created(other_cxt));
  auto packet = get_packet(first_cxt);
  ASSERT_EQ(1u, phv_source->get_created(first_cxt));
  ASSERT_EQ(0u, phv_source->get_created(other_cxt));
}

TEST_F(PacketTest, ChangeContext) {
  const size_t first_cxt = 0;
  const size_t other_cxt = 1;
  auto packet = get_packet(first_cxt);
  ASSERT_EQ(1u, phv_source->get_created(first_cxt));
  ASSERT_EQ(0u, phv_source->get_created(other_cxt));
  packet.change_context(other_cxt);
  ASSERT_EQ(1u, phv_source->get_created(first_cxt));
  ASSERT_EQ(1u, phv_source->get_destroyed(first_cxt));
  ASSERT_EQ(1u, phv_source->get_created(other_cxt));
  ASSERT_EQ(0u, phv_source->get_destroyed(other_cxt));
}

TEST_F(PacketTest, Truncate) {
  const size_t cxt = 0;
  const size_t first_length = 128;
  std::vector<char> data;
  data.reserve(first_length);
  for (size_t i = 0; i < first_length; i++) {
    data.push_back(static_cast<char>(i));
  }

  Packet pkt_1 = Packet::make_new(
      cxt, 0, 0, 0, 0,
      PacketBuffer(first_length, data.data(), first_length),
      phv_source.get());

  const size_t truncated_length_small = 47;
  ASSERT_LT(truncated_length_small, first_length);
  pkt_1.truncate(truncated_length_small);
  ASSERT_EQ(truncated_length_small, pkt_1.get_data_size());
  ASSERT_TRUE(std::equal(
      data.begin(), data.begin() + pkt_1.get_data_size(), pkt_1.data()));

  Packet pkt_2 = Packet::make_new(
      cxt, 0, 0, 0, 0,
      PacketBuffer(first_length, data.data(), first_length),
      phv_source.get());

  const size_t truncated_length_big = 200;
  ASSERT_GT(truncated_length_big, first_length);
  pkt_2.truncate(truncated_length_big);
  ASSERT_EQ(first_length, pkt_2.get_data_size());
  ASSERT_TRUE(std::equal(
      data.begin(), data.begin() + pkt_2.get_data_size(), pkt_2.data()));
}

TEST_F(PacketTest, PacketRegisters) {
  const uint64_t v1 = 0u;
  const uint64_t v2 = 6789u;
  const size_t idx = 0u;
  ASSERT_LT(idx, Packet::nb_registers);
  auto packet = get_packet(0);
  packet.set_register(idx, v1);
  ASSERT_EQ(v1, packet.get_register(idx));
  packet.set_register(idx, v2);
  ASSERT_EQ(v2, packet.get_register(idx));
}

TEST_F(PacketTest, Exit) {
  auto packet = get_packet(0);
  ASSERT_FALSE(packet.is_marked_for_exit());
  packet.mark_for_exit();
  ASSERT_TRUE(packet.is_marked_for_exit());
  packet.reset_exit();
  ASSERT_FALSE(packet.is_marked_for_exit());
}

TEST_F(PacketTest, CopyId) {
  auto packet_0 = std::unique_ptr<Packet>(new Packet(get_packet(0)));
  ASSERT_EQ(0u, packet_0->get_copy_id());
  auto packet_1 = packet_0->clone_with_phv_ptr();
  ASSERT_EQ(1u, packet_1->get_copy_id());
  auto packet_2 = packet_0->clone_with_phv_ptr();
  ASSERT_EQ(2u, packet_2->get_copy_id());

  // reset so now 0.0, 0.2 exist but not 0.1
  // new packet should get 0.3
  packet_1.reset(nullptr);
  auto packet_3 = packet_0->clone_with_phv_ptr();
  ASSERT_EQ(3u, packet_3->get_copy_id());

  packet_3.reset(nullptr);
  packet_2.reset(nullptr);
  packet_0.reset(nullptr);

  auto packet_0_new = std::unique_ptr<Packet>(new Packet(get_packet(0)));
  ASSERT_EQ(0u, packet_0_new->get_copy_id());
  auto packet_1_new = packet_0_new->clone_with_phv_ptr();
  ASSERT_EQ(1u, packet_1_new->get_copy_id());
}
