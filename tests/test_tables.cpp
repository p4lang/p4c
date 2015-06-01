#include <gtest/gtest.h>

#include <memory>
#include "bm_sim/tables.h"

using testing::Types;

using std::string;
using std::to_string;

typedef MatchTableAbstract::ActionEntry ActionEntry;
typedef MatchUnitExact<ActionEntry> MUExact;
typedef MatchUnitLPM<ActionEntry> MULPM;
typedef MatchUnitTernary<ActionEntry> MUTernary;

template <typename MUType>
class TableSizeOne : public ::testing::Test {
protected:
  PHVFactory phv_factory;

  MatchKeyBuilder key_builder;
  std::unique_ptr<MatchTable> table;

  MatchKeyBuilder key_builder_w_valid;
  std::unique_ptr<MatchTable> table_w_valid;

  HeaderType testHeaderType;
  header_id_t testHeader1{0}, testHeader2{1};
  ActionFn action_fn;

  TableSizeOne() 
    : testHeaderType("test_t", 0), action_fn("actionA", 0) {
    testHeaderType.push_back_field("f16", 16);
    testHeaderType.push_back_field("f48", 48);
    phv_factory.push_back_header("test1", testHeader1, testHeaderType);
    phv_factory.push_back_header("test2", testHeader2, testHeaderType);

    key_builder.push_back_field(testHeader1, 0, 16);

    key_builder_w_valid.push_back_field(testHeader1, 0, 16);
    key_builder_w_valid.push_back_valid_header(testHeader2);

    std::unique_ptr<MUType> match_unit;

    // true enables counters
    match_unit = std::unique_ptr<MUType>(new MUType(1, key_builder));
    table = std::unique_ptr<MatchTable>(
      new MatchTable("test_table", 0, std::move(match_unit), true)
    );
    table->set_next_node(0, nullptr);

    match_unit = std::unique_ptr<MUType>(new MUType(1, key_builder_w_valid));
    table_w_valid = std::unique_ptr<MatchTable>(
      new MatchTable("test_table", 0, std::move(match_unit))
    );
    table_w_valid->set_next_node(0, nullptr);
  }

  MatchErrorCode add_entry(const std::string &key,
			   entry_handle_t *handle);

  MatchErrorCode add_entry_w_valid(const std::string &key,
				   bool valid,
				   entry_handle_t *handle);

  Packet get_pkt(int length) {
    // dummy packet, won't be parsed
    return Packet(0, 0, 0, length, PacketBuffer(length * 2));
  }

  virtual void SetUp() {
    Packet::set_phv_factory(phv_factory);
  }

  virtual void TearDown() {
    Packet::unset_phv_factory();
  }
};

template<>
MatchErrorCode
TableSizeOne<MUExact>::add_entry(const std::string &key,
				 entry_handle_t *handle) {
  ActionData action_data;
  std::vector<MatchKeyParam> match_key;
  match_key.emplace_back(MatchKeyParam::Type::EXACT, key);
  return table->add_entry(match_key, &action_fn, action_data, handle);
}

template<>
MatchErrorCode
TableSizeOne<MULPM>::add_entry(const std::string &key,
			       entry_handle_t *handle) {
  ActionData action_data;
  std::vector<MatchKeyParam> match_key;
  int prefix_length = 16;
  match_key.emplace_back(MatchKeyParam::Type::LPM, key, prefix_length);
  return table->add_entry(match_key, &action_fn, action_data, handle);
}

template<>
MatchErrorCode
TableSizeOne<MUTernary>::add_entry(const std::string &key,
				   entry_handle_t *handle) {
  ActionData action_data;
  std::vector<MatchKeyParam> match_key;
  std::string mask = "\xff\xff";
  int priority = 1;
  match_key.emplace_back(MatchKeyParam::Type::TERNARY, key, std::move(mask));
  return table->add_entry(match_key, &action_fn, action_data, handle, priority);
}

template<>
MatchErrorCode
TableSizeOne<MUExact>::add_entry_w_valid(const std::string &key,
					 bool valid,
					 entry_handle_t *handle) {
  ActionData action_data;
  std::vector<MatchKeyParam> match_key;
  match_key.emplace_back(MatchKeyParam::Type::EXACT, key);
  // I'm putting the valid match in second position on purpose
  match_key.emplace_back(MatchKeyParam::Type::VALID, valid ? "\x01" : "\x00");
  return table_w_valid->add_entry(match_key, &action_fn, action_data, handle);
}

template<>
MatchErrorCode
TableSizeOne<MULPM>::add_entry_w_valid(const std::string &key,
				       bool valid,
				       entry_handle_t *handle) {
  ActionData action_data;
  std::vector<MatchKeyParam> match_key;
  int prefix_length = 16;
  match_key.emplace_back(MatchKeyParam::Type::LPM, key, prefix_length);
  match_key.emplace_back(MatchKeyParam::Type::VALID, valid ? "\x01" : "\x00");
  return table_w_valid->add_entry(match_key, &action_fn, action_data, handle);
}

template<>
MatchErrorCode
TableSizeOne<MUTernary>::add_entry_w_valid(const std::string &key,
					   bool valid,
					   entry_handle_t *handle) {
  ActionData action_data;
  std::vector<MatchKeyParam> match_key;
  std::string mask = "\xff\xff";
  int priority = 1;
  match_key.emplace_back(MatchKeyParam::Type::TERNARY, key, std::move(mask));
  match_key.emplace_back(MatchKeyParam::Type::VALID, valid ? "\x01" : "\x00");
  return table_w_valid->add_entry(match_key, &action_fn, action_data,
				  handle, priority);
}

typedef Types<MUExact,
	      MULPM,
	      MUTernary> TableTypes;

TYPED_TEST_CASE(TableSizeOne, TableTypes);

TYPED_TEST(TableSizeOne, AddEntry) {
  std::string key_ = "\xaa\xaa";
  ByteContainer key("0xaaaa");
  entry_handle_t handle_1, handle_2;
  MatchErrorCode rc;

  rc = this->add_entry(key_, &handle_1);
  ASSERT_EQ(rc, MatchErrorCode::SUCCESS);
  ASSERT_EQ(1u, this->table->get_num_entries());

  rc = this->add_entry(key_, &handle_2);
  ASSERT_EQ(MatchErrorCode::TABLE_FULL, rc);
  ASSERT_EQ(1u, this->table->get_num_entries());
}

TYPED_TEST(TableSizeOne, DeleteEntry) {
  std::string key_ = "\xaa\xaa";
  ByteContainer key("0xaaaa");
  entry_handle_t handle;
  MatchErrorCode rc;

  rc = this->add_entry(key_, &handle);
  ASSERT_EQ(MatchErrorCode::SUCCESS, rc);

  rc = this->table->delete_entry(handle);
  ASSERT_EQ(MatchErrorCode::SUCCESS, rc);
  ASSERT_EQ(0u, this->table->get_num_entries());

  rc = this->table->delete_entry(handle);
  ASSERT_EQ(MatchErrorCode::INVALID_HANDLE, rc);

  rc = this->add_entry(key_, &handle);
  ASSERT_EQ(MatchErrorCode::SUCCESS, rc);
}

TYPED_TEST(TableSizeOne, LookupEntry) {
  std::string key_ = "\x0a\xba";
  ByteContainer key("0x0aba");
  entry_handle_t handle;
  MatchErrorCode rc;
  entry_handle_t lookup_handle;
  bool hit;

  Packet pkt = this->get_pkt(64);
  Field &f = pkt.get_phv()->get_field(this->testHeader1, 0);

  rc = this->add_entry(key_, &handle);
  ASSERT_EQ(MatchErrorCode::SUCCESS, rc);

  f.set("0xaba");
  this->table->lookup(pkt, &hit, &lookup_handle);
  ASSERT_TRUE(hit);

  f.set("0xabb");
  this->table->lookup(pkt, &hit, &lookup_handle);
  ASSERT_FALSE(hit);

  rc = this->table->delete_entry(handle);
  ASSERT_EQ(MatchErrorCode::SUCCESS, rc);

  f.set("0xaba");
  this->table->lookup(pkt, &hit, &lookup_handle);
  ASSERT_FALSE(hit);
}

TYPED_TEST(TableSizeOne, ModifyEntry) {
  std::string key_ = "\x0a\xba";
  ByteContainer key("0x0aba");
  entry_handle_t handle;
  MatchErrorCode rc;
  entry_handle_t lookup_handle;
  bool hit;

  Packet pkt = this->get_pkt(64);
  Field &f = pkt.get_phv()->get_field(this->testHeader1, 0);

  rc = this->add_entry(key_, &handle);
  ASSERT_EQ(MatchErrorCode::SUCCESS, rc);

  f.set("0xaba");
  const ActionEntry &entry_1 = this->table->lookup(pkt, &hit, &lookup_handle);
  ASSERT_TRUE(hit);
  ASSERT_EQ(0, entry_1.action_fn.action_data_size());

  // in theory I could modify the entry directly using the pointer, but I need
  // to exercise my modify_entry function

  // we modify the entry, this time using some action data
  ActionData new_action_data;
  new_action_data.push_back_action_data(0xaba);
  this->table->modify_entry(handle, &(this->action_fn),
			    std::move(new_action_data));

  const ActionEntry &entry_2 = this->table->lookup(pkt, &hit, &lookup_handle);
  ASSERT_TRUE(hit);
  ASSERT_NE(0, entry_2.action_fn.action_data_size());
  ASSERT_EQ((unsigned) 0xaba, entry_2.action_fn.get_action_data(0).get_uint());
}

TYPED_TEST(TableSizeOne, Counters) {
  std::string key_ = "\x0a\xba";
  ByteContainer key("0x0aba");
  entry_handle_t handle;
  MatchErrorCode rc;

  rc = this->add_entry(key_, &handle);
  ASSERT_EQ(rc, MatchErrorCode::SUCCESS);
  ASSERT_EQ(1u, this->table->get_num_entries());

  uint64_t counter_bytes = 0;
  uint64_t counter_packets = 0;

  rc = this->table->query_counters(handle, &counter_bytes, &counter_packets);
  ASSERT_EQ(rc, MatchErrorCode::SUCCESS);
  ASSERT_EQ(0u, counter_bytes);
  ASSERT_EQ(0u, counter_packets);

  Packet pkt = this->get_pkt(64);
  Field &f = pkt.get_phv()->get_field(this->testHeader1, 0);
  f.set("0xaba");

  // there is no action specified for the entry, potential for a bug :(
  this->table->apply_action(&pkt);

  rc = this->table->query_counters(handle, &counter_bytes, &counter_packets);
  ASSERT_EQ(rc, MatchErrorCode::SUCCESS);
  ASSERT_EQ(64u, counter_bytes);
  ASSERT_EQ(1u, counter_packets);

  f.set("0xabb");
  this->table->apply_action(&pkt);

  // counters should not have been incremented
  rc = this->table->query_counters(handle, &counter_bytes, &counter_packets);
  ASSERT_EQ(rc, MatchErrorCode::SUCCESS);
  ASSERT_EQ(64u, counter_bytes);
  ASSERT_EQ(1u, counter_packets);
}

TYPED_TEST(TableSizeOne, Valid) {
  std::string key_ = "\x0a\xba";
  bool valid = true;
  entry_handle_t handle;
  MatchErrorCode rc;
  entry_handle_t lookup_handle;
  bool hit;

  rc = this->add_entry_w_valid(key_, valid, &handle);
  ASSERT_EQ(rc, MatchErrorCode::SUCCESS);
  ASSERT_EQ(1u, this->table_w_valid->get_num_entries());

  Packet pkt = this->get_pkt(64);
  Field &f = pkt.get_phv()->get_field(this->testHeader1, 0);
  f.set("0xaba");
  Header &h2 = pkt.get_phv()->get_header(this->testHeader2);
  ASSERT_FALSE(h2.is_valid());

  this->table_w_valid->lookup(pkt, &hit, &lookup_handle);
  ASSERT_FALSE(hit);

  h2.mark_valid();

  this->table_w_valid->lookup(pkt, &hit, &lookup_handle);
  ASSERT_TRUE(hit);
}

class TableIndirect : public ::testing::Test {
protected:
  typedef MatchTableIndirect::mbr_hdl_t mbr_hdl_t;

protected:
  PHVFactory phv_factory;

  MatchKeyBuilder key_builder;
  std::unique_ptr<MatchTableIndirect> table;

  HeaderType testHeaderType;
  header_id_t testHeader1{0}, testHeader2{1};
  ActionFn action_fn;

  static const size_t table_size = 128;

  TableIndirect() 
    : testHeaderType("test_t", 0), action_fn("actionA", 0) {
    testHeaderType.push_back_field("f16", 16);
    testHeaderType.push_back_field("f48", 48);
    phv_factory.push_back_header("test1", testHeader1, testHeaderType);
    phv_factory.push_back_header("test2", testHeader2, testHeaderType);

    key_builder.push_back_field(testHeader1, 0, 16);

    table = MatchTableIndirect::create("exact", "test_table", 0,
				       table_size, key_builder, true);
    table->set_next_node(0, nullptr);
  }

  MatchErrorCode add_entry(const std::string &key,
			   mbr_hdl_t mbr,
			   entry_handle_t *handle) {
    std::vector<MatchKeyParam> match_key;
    match_key.emplace_back(MatchKeyParam::Type::EXACT, key);
    return table->add_entry(match_key, mbr, handle);
  }

  MatchErrorCode add_member(unsigned int data, mbr_hdl_t *mbr) {
    ActionData action_data;
    action_data.push_back_action_data(data);
    return table->add_member(&action_fn, std::move(action_data), mbr);
  }

  Packet get_pkt(int length) {
    // dummy packet, won't be parsed
    return Packet(0, 0, 0, length, PacketBuffer(length * 2));
  }

  virtual void SetUp() {
    Packet::set_phv_factory(phv_factory);
  }

  virtual void TearDown() {
    Packet::unset_phv_factory();
  }
};

TEST_F(TableIndirect, AddMember) {
  MatchErrorCode rc;
  mbr_hdl_t mbr;
  size_t num_mbrs = 128;

  for(unsigned int i = 0; i < num_mbrs; i++) {
    rc = add_member(i, &mbr);
    ASSERT_EQ(rc, MatchErrorCode::SUCCESS);
    // this is implementation specific, is it a good idea ?
    ASSERT_EQ(i, mbr);
    ASSERT_EQ(i + 1, table->get_num_members());  
  }
}

TEST_F(TableIndirect, AddEntry) {
  MatchErrorCode rc;
  mbr_hdl_t mbr_1;
  mbr_hdl_t mbr_2 = 999u;
  entry_handle_t handle;
  std::string key;
  unsigned int data = 666u;

  rc = add_member(data, &mbr_1);
  ASSERT_EQ(rc, MatchErrorCode::SUCCESS);

  rc = add_entry(key, mbr_2, &handle);
  ASSERT_EQ(rc, MatchErrorCode::INVALID_MBR_HANDLE);
  ASSERT_EQ(0u, table->get_num_entries());  

  static_assert(table_size < 256, "");

  for(unsigned int i = 0; i < table_size; i++) {
    key = std::string(2, (char) table_size);
    rc = add_entry(key, mbr_1, &handle);
    ASSERT_EQ(rc, MatchErrorCode::SUCCESS);
    // this is implementation specific, is it a good idea ?
    ASSERT_EQ(i, handle);
    ASSERT_EQ(i + 1, table->get_num_entries());  
  }

  rc = add_entry("\xff\xff", mbr_1, &handle);
  ASSERT_EQ(MatchErrorCode::TABLE_FULL, rc);
}

TEST_F(TableIndirect, DeleteMember) {
  std::string key("\x0a\ba");
  MatchErrorCode rc;
  entry_handle_t handle;
  mbr_hdl_t mbr;

  rc = add_member(0xaba, &mbr);
  ASSERT_EQ(rc, MatchErrorCode::SUCCESS);
  // this is implementation specific, is it a good idea ?
  ASSERT_EQ(0u, mbr);
  ASSERT_EQ(1u, table->get_num_members());

  rc = table->delete_member(mbr);
  ASSERT_EQ(rc, MatchErrorCode::SUCCESS);
  ASSERT_EQ(0u, table->get_num_members());

  rc = add_member(0xabb, &mbr);
  ASSERT_EQ(rc, MatchErrorCode::SUCCESS);
  // this is implementation specific, is it a good idea ?
  ASSERT_EQ(0u, mbr);
  ASSERT_EQ(1u, table->get_num_members());

  rc = add_entry(key, mbr, &handle);
  ASSERT_EQ(MatchErrorCode::SUCCESS, rc);

  rc = table->delete_member(mbr);
  ASSERT_EQ(rc, MatchErrorCode::MBR_STILL_USED);

  rc = table->delete_entry(handle);
  ASSERT_EQ(MatchErrorCode::SUCCESS, rc);

  rc = table->delete_member(mbr);
  ASSERT_EQ(rc, MatchErrorCode::SUCCESS);
}

TEST_F(TableIndirect, ModifyEntry) {
  MatchErrorCode rc;
  mbr_hdl_t mbr_1, mbr_2;
  mbr_hdl_t mbr_3 = 999u;
  entry_handle_t handle;
  std::string key = "\x0a\xba";
  unsigned int data = 666u;

  rc = add_member(data, &mbr_1);
  ASSERT_EQ(rc, MatchErrorCode::SUCCESS);

  rc = add_member(data, &mbr_2);
  ASSERT_EQ(rc, MatchErrorCode::SUCCESS);

  rc = add_entry(key, mbr_1, &handle);
  ASSERT_EQ(rc, MatchErrorCode::SUCCESS);

  rc = table->modify_entry(handle, mbr_2);
  ASSERT_EQ(rc, MatchErrorCode::SUCCESS);

  rc = table->modify_entry(handle, mbr_3);
  ASSERT_EQ(rc, MatchErrorCode::INVALID_MBR_HANDLE);
}

TEST_F(TableIndirect, LookupEntry) {
  std::string key = "\x0a\xba";
  std::string nkey = "\x0a\xbb";
  unsigned int data = 666u;
  mbr_hdl_t mbr;
  entry_handle_t handle;
  MatchErrorCode rc;
  entry_handle_t lookup_handle;
  bool hit;

  Packet pkt = get_pkt(64);
  Field &f = pkt.get_phv()->get_field(testHeader1, 0);

  rc = add_member(data, &mbr);
  ASSERT_EQ(rc, MatchErrorCode::SUCCESS);

  rc = add_entry(key, mbr, &handle);
  ASSERT_EQ(MatchErrorCode::SUCCESS, rc);

  f.set("0xaba");
  const ActionEntry &entry = table->lookup(pkt, &hit, &lookup_handle);
  ASSERT_TRUE(hit);
  ASSERT_EQ(data, entry.action_fn.get_action_data(0).get_uint());

  f.set("0xabb");
  table->lookup(pkt, &hit, &lookup_handle);
  ASSERT_FALSE(hit);

  rc = table->delete_entry(handle);
  ASSERT_EQ(MatchErrorCode::SUCCESS, rc);

  f.set("0xaba");
  table->lookup(pkt, &hit, &lookup_handle);
  ASSERT_FALSE(hit);
}
