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

#include <boost/filesystem.hpp>

#include <bm/bm_sim/switch.h>

using namespace::bm;

namespace fs = boost::filesystem;

class SwitchTest : public Switch {
 public:
  int receive(int port_num, const char *buffer, int len) override {
    (void) port_num; (void) buffer; (void) len;
    return 0;
  }

  void start_and_return() override { }
};

class RuntimeIfaceTest : public ::testing::Test {
 protected:
  SwitchTest sw{};
  size_t cxt_id{0};

  RuntimeIfaceTest() { }

  virtual void SetUp() {
    // load JSON
    fs::path json_path = fs::path(testdata_dir) / fs::path(test_json);
    assert(sw.init_objects(json_path.string()) == 0);
  }

  // virtual void TearDown() { }

 private:
  static const std::string testdata_dir;
  static const std::string test_json;
};

const std::string RuntimeIfaceTest::testdata_dir = TESTDATADIR;
const std::string RuntimeIfaceTest::test_json = "runtime_iface.json";

TEST_F(RuntimeIfaceTest, Counters) {
  using ErrorCode = Counter::CounterErrorCode;

  ErrorCode rc;
  MatchTableAbstract::counter_value_t bytes = 0;
  MatchTableAbstract::counter_value_t packets = 0;
  size_t good_idx = 0, bad_idx = 1024;
  std::string good_name("my_indirect_counter");
  std::string bad_name("bad_counter_name");

  rc = sw.read_counters(cxt_id, good_name, good_idx, &bytes, &packets);
  ASSERT_EQ(ErrorCode::SUCCESS, rc);
  rc = sw.read_counters(cxt_id, bad_name, good_idx, &bytes, &packets);
  ASSERT_EQ(ErrorCode::INVALID_COUNTER_NAME, rc);
  rc = sw.read_counters(cxt_id, good_name, bad_idx, &bytes, &packets);
  ASSERT_EQ(ErrorCode::INVALID_INDEX, rc);

  rc = sw.write_counters(cxt_id, good_name, good_idx, bytes, packets);
  ASSERT_EQ(ErrorCode::SUCCESS, rc);
  rc = sw.write_counters(cxt_id, bad_name, good_idx, bytes, packets);
  ASSERT_EQ(ErrorCode::INVALID_COUNTER_NAME, rc);
  rc = sw.write_counters(cxt_id, good_name, bad_idx, bytes, packets);
  ASSERT_EQ(ErrorCode::INVALID_INDEX, rc);

  rc = sw.reset_counters(cxt_id, good_name);
  ASSERT_EQ(ErrorCode::SUCCESS, rc);
}

TEST_F(RuntimeIfaceTest, Meters) {
  using ErrorCode = Meter::MeterErrorCode;

  ErrorCode rc;

  // committed : 2 packets per second, burst size of 3
  Meter::rate_config_t committed_rate = {0.000002, 3};
  // peak : 10 packets per second, burst size of 1
  Meter::rate_config_t peak_rate = {0.00001, 1};
  auto good_config = {committed_rate, peak_rate};

  size_t good_idx = 0, bad_idx = 1024;
  std::string good_name("my_indirect_meter");
  std::string bad_name("bad_meter_name");

  rc = sw.meter_array_set_rates(cxt_id, good_name, good_config);
  ASSERT_EQ(ErrorCode::SUCCESS, rc);
  rc = sw.meter_array_set_rates(cxt_id, bad_name, good_config);
  ASSERT_EQ(ErrorCode::INVALID_METER_NAME, rc);
  rc = sw.meter_array_set_rates(cxt_id, good_name, {peak_rate, committed_rate});
  ASSERT_EQ(ErrorCode::INVALID_INFO_RATE_VALUE, rc);
  rc = sw.meter_array_set_rates(cxt_id, good_name, {peak_rate});
  ASSERT_EQ(ErrorCode::BAD_RATES_LIST, rc);

  rc = sw.meter_set_rates(cxt_id, good_name, good_idx, good_config);
  ASSERT_EQ(ErrorCode::SUCCESS, rc);
  rc = sw.meter_set_rates(cxt_id, bad_name, good_idx, good_config);
  ASSERT_EQ(ErrorCode::INVALID_METER_NAME, rc);
  rc = sw.meter_set_rates(cxt_id, good_name, bad_idx, good_config);
  ASSERT_EQ(ErrorCode::INVALID_INDEX, rc);
}

TEST_F(RuntimeIfaceTest, Registers) {
  using ErrorCode = Register::RegisterErrorCode;

  ErrorCode rc;
  Data v(0);
  size_t good_idx = 0, bad_idx = 1024;
  std::string good_name("my_register");
  std::string bad_name("bad_register_name");

  rc = sw.register_read(cxt_id, good_name, good_idx, &v);
  ASSERT_EQ(ErrorCode::SUCCESS, rc);
  rc = sw.register_read(cxt_id, bad_name, good_idx, &v);
  ASSERT_EQ(ErrorCode::INVALID_REGISTER_NAME, rc);
  rc = sw.register_read(cxt_id, good_name, bad_idx, &v);
  ASSERT_EQ(ErrorCode::INVALID_INDEX, rc);

  rc = sw.register_write(cxt_id, good_name, good_idx, v);
  ASSERT_EQ(ErrorCode::SUCCESS, rc);
  rc = sw.register_write(cxt_id, bad_name, good_idx, v);
  ASSERT_EQ(ErrorCode::INVALID_REGISTER_NAME, rc);
  rc = sw.register_write(cxt_id, good_name, bad_idx, v);
  ASSERT_EQ(ErrorCode::INVALID_INDEX, rc);

  rc = sw.register_write_range(cxt_id, good_name, good_idx, good_idx + 1, v);
  ASSERT_EQ(ErrorCode::SUCCESS, rc);
  rc = sw.register_write_range(cxt_id, bad_name, good_idx, good_idx + 1, v);
  ASSERT_EQ(ErrorCode::INVALID_REGISTER_NAME, rc);
  rc = sw.register_write_range(cxt_id, good_name, good_idx, bad_idx, v);
  ASSERT_EQ(ErrorCode::INVALID_INDEX, rc);
  rc = sw.register_write_range(cxt_id, good_name, good_idx + 1, good_idx, v);
  ASSERT_EQ(ErrorCode::INVALID_INDEX, rc);

  rc = sw.register_reset(cxt_id, good_name);
  ASSERT_EQ(ErrorCode::SUCCESS, rc);
  rc = sw.register_reset(cxt_id, bad_name);
  ASSERT_EQ(ErrorCode::INVALID_REGISTER_NAME, rc);
}
