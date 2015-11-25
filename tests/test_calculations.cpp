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

#include <random>

#include "bm_sim/calculations.h"
#include "bm_sim/parser.h"

// Google Test fixture for calculations tests
class CalculationTest : public ::testing::Test {
protected:
  PHVFactory phv_factory;

  HeaderType testHeaderType;
  header_id_t testHeader1{0}, testHeader2{1};

  ParseState oneParseState;
  Parser parser;

  // I am not seeding this on purpose, at least for now
  std::mt19937 gen;
  std::uniform_int_distribution<unsigned char> dis;

  static constexpr size_t header_size = 19;

  CalculationTest()
    : testHeaderType("test_t", 0),
      oneParseState("parse_state", 0),
      parser("test_parser", 0)
  {
    testHeaderType.push_back_field("f16", 16);
    testHeaderType.push_back_field("f48", 48);
    testHeaderType.push_back_field("f32_1", 32);
    testHeaderType.push_back_field("f32_2", 32);
    testHeaderType.push_back_field("f5", 5);
    testHeaderType.push_back_field("f19", 19);

    phv_factory.push_back_header("test1", testHeader1, testHeaderType);
    phv_factory.push_back_header("test2", testHeader2, testHeaderType);
  }

  Packet get_pkt(const char *data, size_t data_size) {
    // dummy packet, won't be parsed
    return Packet(0, 0, 0, data_size,
		  PacketBuffer(data_size * 2, data, data_size));
  }

  virtual void SetUp() {
    Packet::set_phv_factory(phv_factory);

    oneParseState.add_extract(testHeader1);
    oneParseState.add_extract(testHeader2);
    parser.set_init_state(&oneParseState);
  }

  virtual void TearDown() {
    Packet::unset_phv_factory();
  }
};

TEST_F(CalculationTest, SimpleTest) {
  BufBuilder builder;

  builder.push_back_field(testHeader1, 0); // f16
  builder.push_back_field(testHeader1, 1); // f48
  builder.push_back_field(testHeader1, 3); // f32_2

  builder.push_back_field(testHeader2, 2); // f32_1

  Calculation calc(builder, "xxh64");

  unsigned char pkt_buf[2 * header_size];

  for(size_t i = 0; i < sizeof(pkt_buf); i++) {
    pkt_buf[i] = dis(gen);
  }

  Packet pkt = get_pkt((const char *) pkt_buf, sizeof(pkt_buf));
  parser.parse(&pkt);

  /* A bit primitive, I have to  build the buffer myself */
  unsigned char expected_buf[16];
  std::copy(&pkt_buf[0], &pkt_buf[2], &expected_buf[0]);
  std::copy(&pkt_buf[2], &pkt_buf[8], &expected_buf[2]);
  std::copy(&pkt_buf[12], &pkt_buf[16], &expected_buf[8]);
  std::copy(&pkt_buf[header_size + 8], &pkt_buf[header_size + 12],
	    &expected_buf[12]);

  auto expected = hash::xxh64((const char *) expected_buf, sizeof(expected_buf));
  auto actual = calc.output(pkt);

  ASSERT_EQ(expected, actual);
}

TEST_F(CalculationTest, NonAlignedTest) {
  BufBuilder builder;

  builder.push_back_field(testHeader1, 4); // f5
  builder.push_back_field(testHeader1, 5); // f19

  Calculation calc(builder, "xxh64");;

  unsigned char pkt_buf[2 * header_size];

  for(size_t i = 0; i < sizeof(pkt_buf); i++) {
    pkt_buf[i] = dis(gen);
  }

  Packet pkt = get_pkt((const char *) pkt_buf, sizeof(pkt_buf));
  parser.parse(&pkt);

  /* A bit primitive, I have to  build the buffer myself */
  unsigned char expected_buf[3];
  std::copy(&pkt_buf[16], &pkt_buf[19], &expected_buf[0]);

  auto expected = hash::xxh64((const char *) expected_buf, sizeof(expected_buf));
  auto actual = calc.output(pkt);

  ASSERT_EQ(expected, actual);
}

TEST_F(CalculationTest, WithConstant) {
  BufBuilder builder;

  ByteContainer constant("0x12ab");

  builder.push_back_field(testHeader1, 0); // f16
  builder.push_back_field(testHeader1, 1); // f48
  builder.push_back_constant(constant, 16);
  builder.push_back_field(testHeader1, 3); // f32_2

  Calculation calc(builder, "xxh64");

  unsigned char pkt_buf[2 * header_size];

  for(size_t i = 0; i < sizeof(pkt_buf); i++) {
    pkt_buf[i] = dis(gen);
  }

  Packet pkt = get_pkt((const char *) pkt_buf, sizeof(pkt_buf));
  parser.parse(&pkt);

  /* A bit primitive, I have to  build the buffer myself */
  unsigned char expected_buf[14];
  std::copy(&pkt_buf[0], &pkt_buf[2], &expected_buf[0]);
  std::copy(&pkt_buf[2], &pkt_buf[8], &expected_buf[2]);
  std::copy(constant.begin(), constant.end(), &expected_buf[8]);
  std::copy(&pkt_buf[12], &pkt_buf[16], &expected_buf[10]);

  auto expected = hash::xxh64((const char *) expected_buf, sizeof(expected_buf));
  auto actual = calc.output(pkt);

  ASSERT_EQ(expected, actual);
}

TEST_F(CalculationTest, WithPayload) {
  BufBuilder builder;

  builder.push_back_field(testHeader1, 0); // f16
  builder.push_back_field(testHeader1, 1); // f48
  builder.push_back_field(testHeader1, 3); // f32_2
  builder.push_back_header(testHeader2);
  builder.append_payload();

  Calculation calc(builder, "xxh64");

  unsigned char pkt_buf[2 * header_size + 196];

  for(size_t i = 0; i < sizeof(pkt_buf); i++) {
    pkt_buf[i] = dis(gen);
  }

  Packet pkt = get_pkt((const char *) pkt_buf, sizeof(pkt_buf));
  parser.parse(&pkt);

  /* A bit primitive, I have to  build the buffer myself */
  unsigned char expected_buf[12 + header_size + 196];
  std::copy(&pkt_buf[0], &pkt_buf[2], &expected_buf[0]);
  std::copy(&pkt_buf[2], &pkt_buf[8], &expected_buf[2]);
  std::copy(&pkt_buf[12], &pkt_buf[16], &expected_buf[8]);
  std::copy(&pkt_buf[header_size], &pkt_buf[sizeof(pkt_buf)],
	    &expected_buf[12]);

  auto expected = hash::xxh64((const char *) expected_buf, sizeof(expected_buf));
  auto actual = calc.output(pkt);

  ASSERT_EQ(expected, actual);
}


namespace {

struct Hash {
  uint32_t operator()(const char *buffer, size_t s) const {
    (void) buffer; (void) s;
    return 0u;
  }
};

REGISTER_HASH(Hash);

}

TEST(CalculationsMap, Test) {
  ASSERT_NE(nullptr, CalculationsMap::get_instance()->get_copy("Hash"));

  ASSERT_EQ(nullptr, CalculationsMap::get_instance()->get_copy("Hash_Neg"));
}
