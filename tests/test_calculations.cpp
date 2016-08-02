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

#include <bm/bm_sim/calculations.h>
#include <bm/bm_sim/parser.h>

using namespace bm;

using testing::Types;

// Google Test fixture for calculations tests
class CalculationTest : public ::testing::Test {
 protected:
  PHVFactory phv_factory;

  HeaderType testHeaderType, testHeaderType2;
  header_id_t testHeader1{0}, testHeader2{1}, testHeader3{2};

  ParseState oneParseState;
  Parser parser;

  std::unique_ptr<PHVSourceIface> phv_source{nullptr};

  // I am not seeding this on purpose, at least for now
  std::mt19937 gen;
  std::uniform_int_distribution<unsigned char> dis;

  static constexpr size_t header_size = 19;

  CalculationTest()
      : testHeaderType("test_t", 0),
        testHeaderType2("test2_t", 1),
        oneParseState("parse_state", 0),
        parser("test_parser", 0),
        phv_source(PHVSourceIface::make_phv_source()) {
    testHeaderType.push_back_field("f16", 16);
    testHeaderType.push_back_field("f48", 48);
    testHeaderType.push_back_field("f32_1", 32);
    testHeaderType.push_back_field("f32_2", 32);
    testHeaderType.push_back_field("f5", 5);
    testHeaderType.push_back_field("f19", 19);

    testHeaderType2.push_back_field("f6", 6);
    testHeaderType2.push_back_field("f8", 8);
    testHeaderType2.push_back_field("f9", 9);
    testHeaderType2.push_back_field("f9", 9);

    phv_factory.push_back_header("test1", testHeader1, testHeaderType);
    phv_factory.push_back_header("test2", testHeader2, testHeaderType);
    phv_factory.push_back_header("test3", testHeader3, testHeaderType2);
  }

  Packet get_pkt(const char *data, size_t data_size) {
    // dummy packet, won't be parsed
    return Packet::make_new(
        data_size, PacketBuffer(data_size * 2, data, data_size),
        phv_source.get());
  }

  virtual void SetUp() {
    phv_source->set_phv_factory(0, &phv_factory);

    oneParseState.add_extract(testHeader1);
    oneParseState.add_extract(testHeader2);
    parser.set_init_state(&oneParseState);
  }

  // virtual void TearDown() { }
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

  auto expected = hash::xxh64(
      reinterpret_cast<const char *>(expected_buf), sizeof(expected_buf));
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

// this test helped catch a bug in Field::deparse (for some alignments,
// deparsing was overwriting previous bits)
TEST_F(CalculationTest, Extra) {
  BufBuilder builder;

  builder.push_back_field(testHeader3, 0); // f6
  builder.push_back_field(testHeader3, 1); // f8
  builder.push_back_field(testHeader3, 2); // f9
  builder.push_back_field(testHeader3, 3); // f9

  Calculation calc(builder, "identity");

  unsigned char pkt_buf[128];  // dummy, not used
  Packet pkt = get_pkt((const char *) pkt_buf, sizeof(pkt_buf));
  PHV *phv = pkt.get_phv();
  auto &hdr = phv->get_header(testHeader3);
  hdr.mark_valid();
  hdr.get_field(0).set(0x3d);
  hdr.get_field(1).set(0x3e);
  hdr.get_field(2).set(1);
  hdr.get_field(3).set(0);

  unsigned int expected = 0xf4f80200;
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

// Could use a templatized test for this, but is it worth it?

TEST(HashTest, Identity) {
  const auto ptr = CalculationsMap::get_instance()->get_copy("identity");
  ASSERT_NE(nullptr, ptr);

  const char input_buffer[] = {1, 2, 3, 4, 5};
  const uint64_t expected = 0x0102030405;

  const uint64_t output = ptr->output(input_buffer, sizeof(input_buffer));

  ASSERT_EQ(expected, output);
}

TEST(HashTest, Cksum16) {
  const auto ptr = CalculationsMap::get_instance()->get_copy("cksum16");
  ASSERT_NE(nullptr, ptr);

  // taken from wikipedia:
  // https://en.wikipedia.org/wiki/IPv4_header_checksum
  const unsigned char input_buffer[] = {0x45, 0x00, 0x00, 0x73, 0x00, 0x00,
                                        0x40, 0x00, 0x40, 0x11, 0x00, 0x00,
                                        0xc0, 0xa8, 0x00, 0x01, 0xc0, 0xa8,
                                        0x00, 0xc7};
  const uint16_t expected = 0xb861;

  const uint16_t output = ptr->output(
      reinterpret_cast<const char *>(input_buffer), sizeof(input_buffer));

  ASSERT_EQ(expected, output);
}

TEST(HashTest, Crc16) {
  const auto ptr = CalculationsMap::get_instance()->get_copy("crc16");
  ASSERT_NE(nullptr, ptr);

  const unsigned char input_buffer[] = {0x0b, 0xb8, 0x1f, 0x90};
  const uint16_t expected = 0x5d8a;

  const uint16_t output = ptr->output(
      reinterpret_cast<const char *>(input_buffer), sizeof(input_buffer));

  ASSERT_EQ(expected, output);
}

TEST(HashTest, CrcCCITT) {
  const auto ptr = CalculationsMap::get_instance()->get_copy("crcCCITT");
  ASSERT_NE(nullptr, ptr);

  const unsigned char input_buffer[] = {0x0b, 0xb8, 0x1f, 0x90};
  const uint16_t expected = 0x5d75;

  const uint16_t output = ptr->output(
      reinterpret_cast<const char *>(input_buffer), sizeof(input_buffer));

  ASSERT_EQ(expected, output);
}

TEST(HashTest, Crc32) {
  const auto ptr = CalculationsMap::get_instance()->get_copy("crc32");
  ASSERT_NE(nullptr, ptr);

  const unsigned char input_buffer[] = {0x0b, 0xb8, 0x1f, 0x90};
  const uint32_t expected = 0x005d6a6f;

  const uint32_t output = ptr->output(
      reinterpret_cast<const char *>(input_buffer), sizeof(input_buffer));

  ASSERT_EQ(expected, output);
}

TEST(HashTest, Crc16Custom) {
  typedef CustomCrcMgr<uint16_t>::crc_config_t crc16_config_t;

  auto ptr = CalculationsMap::get_instance()->get_copy("crc16_custom");
  ASSERT_NE(nullptr, ptr);

  uint16_t output;

  const unsigned char input_buffer[] = {0x0b, 0xb8, 0x1f, 0x90};
  const char *buffer_ = reinterpret_cast<const char *>(input_buffer);

  // by default, standard crc16
  const uint16_t expected_crc16 = 0x5d8a;
  output = ptr->output(buffer_, sizeof(input_buffer));
  ASSERT_EQ(expected_crc16, output);

  // set again to standard crc16, same result
  ASSERT_EQ(CustomCrcErrorCode::SUCCESS,
            CustomCrcMgr<uint16_t>::update_config(
                ptr.get(), {0x8005, 0x0000, 0x0000, true, true}));
  output = ptr->output(buffer_, sizeof(input_buffer));
  ASSERT_EQ(expected_crc16, output);

  // change to crcCCITT
  const uint16_t expected_crcCCITT = 0x5d75;
  ASSERT_EQ(CustomCrcErrorCode::SUCCESS,
            CustomCrcMgr<uint16_t>::update_config(
                ptr.get(), {0x1021, 0xffff, 0x0000, false, false}));
  output = ptr->output(buffer_, sizeof(input_buffer));
  ASSERT_EQ(expected_crcCCITT, output);

  // change to crc16 xmodem
  const uint16_t expected_crcXMODEM = 0xd9b5;
  ASSERT_EQ(CustomCrcErrorCode::SUCCESS,
            CustomCrcMgr<uint16_t>::update_config(
                ptr.get(), {0x1021, 0x0000, 0x0000, false, false}));
  output = ptr->output(buffer_, sizeof(input_buffer));
  ASSERT_EQ(expected_crcXMODEM, output);

  // trying to update a crc16 checksum as if it was a crc32 one fails
  ASSERT_EQ(CustomCrcErrorCode::WRONG_TYPE_CALCULATION,
            CustomCrcMgr<uint32_t>::update_config(
                ptr.get(), {0, 0, 0, true, true}));
}

TEST(HashTest, Crc32Custom) {
  typedef CustomCrcMgr<uint32_t>::crc_config_t crc32_config_t;

  auto ptr = CalculationsMap::get_instance()->get_copy("crc32_custom");
  ASSERT_NE(nullptr, ptr);

  uint32_t output;

  const unsigned char input_buffer[] = {0x0b, 0xb8, 0x1f, 0x90};
  const char *buffer_ = reinterpret_cast<const char *>(input_buffer);

  // by default, standard crc32
  const uint32_t expected_crc32 = 0x005d6a6f;
  output = ptr->output(buffer_, sizeof(input_buffer));
  ASSERT_EQ(expected_crc32, output);

  // set again to standard crc32, same result
  ASSERT_EQ(CustomCrcErrorCode::SUCCESS,
            CustomCrcMgr<uint32_t>::update_config(
                ptr.get(), {0x4c11db7, 0xffffffff, 0xffffffff, true, true}));
  output = ptr->output(buffer_, sizeof(input_buffer));
  ASSERT_EQ(expected_crc32, output);

  // change to crc32 mpeg2
  const uint32_t expected_crcMPEG2 = 0x125e1592;
  ASSERT_EQ(CustomCrcErrorCode::SUCCESS,
            CustomCrcMgr<uint32_t>::update_config(
                ptr.get(), {0x4c11db7, 0xffffffff, 0x00000000, false, false}));
  output = ptr->output(buffer_, sizeof(input_buffer));
  ASSERT_EQ(expected_crcMPEG2, output);

  // trying to update a crc32 checksum as if it was a crc16 one fails
  ASSERT_EQ(CustomCrcErrorCode::WRONG_TYPE_CALCULATION,
            CustomCrcMgr<uint16_t>::update_config(
                ptr.get(), {0, 0, 0, true, true}));
}
