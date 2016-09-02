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

#include <fstream>
#include <sstream>
#include <string>

#include <boost/filesystem.hpp>

#include <bm/bm_sim/P4Objects.h>

using namespace bm;

namespace fs = boost::filesystem;

/* I need to find a better way to test the json parser, maybe I could simply
   read from the target files... */

const std::string JSON_TEST_STRING_1 =
  "{\"header_types\":[{\"name\":\"standard_metadata_t\",\"id\":0,\"fields\":[[\"ingress_port\",9],[\"packet_length\",32],[\"egress_spec\",9],[\"egress_port\",9],[\"egress_instance\",32],[\"instance_type\",32],[\"clone_spec\",32],[\"_padding\",5]]},{\"name\":\"ethernet_t\",\"id\":1,\"fields\":[[\"dstAddr\",48],[\"srcAddr\",48],[\"etherType\",16]]},{\"name\":\"ipv4_t\",\"id\":2,\"fields\":[[\"version\",4],[\"ihl\",4],[\"diffserv\",8],[\"totalLen\",16],[\"identification\",16],[\"flags\",3],[\"fragOffset\",13],[\"ttl\",8],[\"protocol\",8],[\"hdrChecksum\",16],[\"srcAddr\",32],[\"dstAddr\",32]]},{\"name\":\"routing_metadata_t\",\"id\":3,\"fields\":[[\"nhop_ipv4\",32]]}],\"headers\":[{\"name\":\"standard_metadata\",\"id\":0,\"header_type\":\"standard_metadata_t\"},{\"name\":\"ethernet\",\"id\":1,\"header_type\":\"ethernet_t\"},{\"name\":\"ipv4\",\"id\":2,\"header_type\":\"ipv4_t\"},{\"name\":\"routing_metadata\",\"id\":3,\"header_type\":\"routing_metadata_t\"}],\"header_stacks\":[],\"parsers\":[{\"name\":\"parser\",\"id\":0,\"init_state\":\"start\",\"parse_states\":[{\"name\":\"start\",\"id\":0,\"parser_ops\":[],\"transition_key\":[],\"transitions\":[{\"value\":\"default\",\"mask\":null,\"next_state\":\"parse_ethernet\"}]},{\"name\":\"parse_ethernet\",\"id\":1,\"parser_ops\":[{\"op\":\"extract\",\"parameters\":[{\"type\":\"regular\",\"value\":\"ethernet\"}]}],\"transition_key\":[{\"type\":\"field\",\"value\":[\"ethernet\",\"etherType\"]}],\"transitions\":[{\"value\":\"0x800\",\"mask\":null,\"next_state\":\"parse_ipv4\"},{\"value\":\"default\",\"mask\":null,\"next_state\":null}]},{\"name\":\"parse_ipv4\",\"id\":2,\"parser_ops\":[{\"op\":\"extract\",\"parameters\":[{\"type\":\"regular\",\"value\":\"ipv4\"}]}],\"transition_key\":[],\"transitions\":[{\"value\":\"default\",\"mask\":null,\"next_state\":null}]}]}],\"deparsers\":[{\"name\":\"deparser\",\"id\":0,\"order\":[\"ethernet\",\"ipv4\"]}],\"meter_arrays\":[],\"actions\":[{\"name\":\"set_nhop\",\"id\":0,\"runtime_data\":[{\"name\":\"nhop_ipv4\",\"bitwidth\":32},{\"name\":\"port\",\"bitwidth\":9}],\"primitives\":[{\"op\":\"modify_field\",\"parameters\":[{\"type\":\"field\",\"value\":[\"routing_metadata\",\"nhop_ipv4\"]},{\"type\":\"runtime_data\",\"value\":0}]},{\"op\":\"modify_field\",\"parameters\":[{\"type\":\"field\",\"value\":[\"standard_metadata\",\"egress_port\"]},{\"type\":\"runtime_data\",\"value\":1}]},{\"op\":\"add_to_field\",\"parameters\":[{\"type\":\"field\",\"value\":[\"ipv4\",\"ttl\"]},{\"type\":\"hexstr\",\"value\":\"-0x1\"}]}]},{\"name\":\"rewrite_mac\",\"id\":1,\"runtime_data\":[{\"name\":\"smac\",\"bitwidth\":48}],\"primitives\":[{\"op\":\"modify_field\",\"parameters\":[{\"type\":\"field\",\"value\":[\"ethernet\",\"srcAddr\"]},{\"type\":\"runtime_data\",\"value\":0}]}]},{\"name\":\"_drop\",\"id\":2,\"runtime_data\":[],\"primitives\":[{\"op\":\"drop\",\"parameters\":[]}]},{\"name\":\"set_dmac\",\"id\":3,\"runtime_data\":[{\"name\":\"dmac\",\"bitwidth\":48}],\"primitives\":[{\"op\":\"modify_field\",\"parameters\":[{\"type\":\"field\",\"value\":[\"ethernet\",\"dstAddr\"]},{\"type\":\"runtime_data\",\"value\":0}]}]}],\"pipelines\":[{\"name\":\"ingress\",\"id\":0,\"init_table\":\"_condition_0\",\"tables\":[{\"name\":\"ipv4_lpm\",\"id\":0,\"match_type\":\"lpm\",\"type\":\"simple\",\"max_size\":1024,\"with_counters\":false,\"key\":[{\"match_type\":\"lpm\",\"target\":[\"ipv4\",\"dstAddr\"]}],\"actions\":[\"set_nhop\",\"_drop\"],\"next_tables\":{\"set_nhop\":\"forward\",\"_drop\":\"forward\"},\"default_action\":null},{\"name\":\"forward\",\"id\":1,\"match_type\":\"exact\",\"type\":\"simple\",\"max_size\":512,\"with_counters\":false,\"key\":[{\"match_type\":\"exact\",\"target\":[\"routing_metadata\",\"nhop_ipv4\"]}],\"actions\":[\"set_dmac\",\"_drop\"],\"next_tables\":{\"set_dmac\":null,\"_drop\":null},\"default_action\":null}],\"conditionals\":[{\"name\":\"_condition_0\",\"id\":0,\"expression\":{\"type\":\"expression\",\"value\":{\"op\":\"and\",\"left\":{\"type\":\"expression\",\"value\":{\"op\":\"valid\",\"left\":null,\"right\":{\"type\":\"header\",\"value\":\"ipv4\"}}},\"right\":{\"type\":\"expression\",\"value\":{\"op\":\">\",\"left\":{\"type\":\"field\",\"value\":[\"ipv4\",\"ttl\"]},\"right\":{\"type\":\"hexstr\",\"value\":\"0x0\"}}}}},\"true_next\":\"ipv4_lpm\",\"false_next\":null}]},{\"name\":\"egress\",\"id\":1,\"init_table\":\"send_frame\",\"tables\":[{\"name\":\"send_frame\",\"id\":2,\"match_type\":\"exact\",\"type\":\"simple\",\"max_size\":256,\"with_counters\":false,\"key\":[{\"match_type\":\"exact\",\"target\":[\"standard_metadata\",\"egress_port\"]}],\"actions\":[\"rewrite_mac\",\"_drop\"],\"next_tables\":{\"rewrite_mac\":null,\"_drop\":null},\"default_action\":null}],\"conditionals\":[]}],\"calculations\":[{\"name\":\"ipv4_checksum\",\"id\":0,\"input\":[{\"type\":\"field\",\"value\":[\"ipv4\",\"version\"]},{\"type\":\"field\",\"value\":[\"ipv4\",\"ihl\"]},{\"type\":\"field\",\"value\":[\"ipv4\",\"diffserv\"]},{\"type\":\"field\",\"value\":[\"ipv4\",\"totalLen\"]},{\"type\":\"field\",\"value\":[\"ipv4\",\"identification\"]},{\"type\":\"field\",\"value\":[\"ipv4\",\"flags\"]},{\"type\":\"field\",\"value\":[\"ipv4\",\"fragOffset\"]},{\"type\":\"field\",\"value\":[\"ipv4\",\"ttl\"]},{\"type\":\"field\",\"value\":[\"ipv4\",\"protocol\"]},{\"type\":\"field\",\"value\":[\"ipv4\",\"srcAddr\"]},{\"type\":\"field\",\"value\":[\"ipv4\",\"dstAddr\"]}],\"algo\":\"csum16\"}],\"checksums\":[{\"name\":\"ipv4.hdrChecksum\",\"id\":0,\"target\":[\"ipv4\",\"hdrChecksum\"],\"type\":\"ipv4\"}],\"learn_lists\":[]}";

const std::string JSON_TEST_STRING_2 =
  "{\"header_types\":[{\"name\":\"standard_metadata_t\",\"id\":0,\"fields\":[[\"ingress_port\",9],[\"packet_length\",32],[\"egress_spec\",9],[\"egress_port\",9],[\"egress_instance\",32],[\"instance_type\",32],[\"clone_spec\",32],[\"_padding\",5]]},{\"name\":\"header_test_t\",\"id\":1,\"fields\":[[\"field8\",8],[\"field16\",16],[\"field20\",20],[\"field24\",24],[\"field32\",32],[\"field48\",48],[\"field64\",64],[\"_padding\",4]]}],\"headers\":[{\"name\":\"standard_metadata\",\"id\":0,\"header_type\":\"standard_metadata_t\"},{\"name\":\"header_test\",\"id\":1,\"header_type\":\"header_test_t\"},{\"name\":\"header_test_1\",\"id\":2,\"header_type\":\"header_test_t\"}],\"header_stacks\":[],\"parsers\":[{\"name\":\"parser\",\"id\":0,\"init_state\":\"start\",\"parse_states\":[{\"name\":\"start\",\"id\":0,\"parser_ops\":[],\"transition_key\":[],\"transitions\":[{\"value\":\"default\",\"mask\":null,\"next_state\":null}]}]}],\"deparsers\":[{\"name\":\"deparser\",\"id\":0,\"order\":[]}],\"meter_arrays\":[],\"actions\":[{\"name\":\"actionB\",\"id\":0,\"runtime_data\":[{\"name\":\"param\",\"bitwidth\":8}],\"primitives\":[{\"op\":\"modify_field\",\"parameters\":[{\"type\":\"field\",\"value\":[\"header_test\",\"field8\"]},{\"type\":\"runtime_data\",\"value\":0}]}]},{\"name\":\"actionA\",\"id\":1,\"runtime_data\":[{\"name\":\"param\",\"bitwidth\":48}],\"primitives\":[{\"op\":\"modify_field\",\"parameters\":[{\"type\":\"field\",\"value\":[\"header_test\",\"field48\"]},{\"type\":\"runtime_data\",\"value\":0}]}]},{\"name\":\"ActionLearn\",\"id\":2,\"runtime_data\":[],\"primitives\":[{\"op\":\"generate_digest\",\"parameters\":[{\"type\":\"hexstr\",\"value\":\"0x1\"},{\"type\":\"hexstr\",\"value\":\"0x1\"}]}]}],\"pipelines\":[{\"name\":\"ingress\",\"id\":0,\"init_table\":\"ExactOne\",\"tables\":[{\"name\":\"ExactOne\",\"id\":0,\"match_type\":\"exact\",\"type\":\"simple\",\"max_size\":512,\"with_counters\":true,\"key\":[{\"match_type\":\"exact\",\"target\":[\"header_test\",\"field32\"]}],\"actions\":[\"actionA\",\"actionB\"],\"next_tables\":{\"actionA\":\"LpmOne\",\"actionB\":\"LpmOne\"},\"default_action\":null},{\"name\":\"LpmOne\",\"id\":1,\"match_type\":\"lpm\",\"type\":\"simple\",\"max_size\":512,\"with_counters\":false,\"key\":[{\"match_type\":\"lpm\",\"target\":[\"header_test\",\"field32\"]}],\"actions\":[\"actionA\"],\"next_tables\":{\"actionA\":\"TernaryOne\"},\"default_action\":null},{\"name\":\"TernaryOne\",\"id\":2,\"match_type\":\"ternary\",\"type\":\"simple\",\"max_size\":512,\"with_counters\":false,\"key\":[{\"match_type\":\"ternary\",\"target\":[\"header_test\",\"field32\"]}],\"actions\":[\"actionA\"],\"next_tables\":{\"actionA\":\"ExactOneNA\"},\"default_action\":null},{\"name\":\"ExactOneNA\",\"id\":3,\"match_type\":\"exact\",\"type\":\"simple\",\"max_size\":512,\"with_counters\":false,\"key\":[{\"match_type\":\"exact\",\"target\":[\"header_test\",\"field20\"]}],\"actions\":[\"actionA\"],\"next_tables\":{\"actionA\":\"ExactTwo\"},\"default_action\":null},{\"name\":\"ExactTwo\",\"id\":4,\"match_type\":\"exact\",\"type\":\"simple\",\"max_size\":512,\"with_counters\":false,\"key\":[{\"match_type\":\"exact\",\"target\":[\"header_test\",\"field32\"]},{\"match_type\":\"exact\",\"target\":[\"header_test\",\"field16\"]}],\"actions\":[\"actionA\"],\"next_tables\":{\"actionA\":\"ExactAndValid\"},\"default_action\":null},{\"name\":\"ExactAndValid\",\"id\":5,\"match_type\":\"exact\",\"type\":\"simple\",\"max_size\":512,\"with_counters\":false,\"key\":[{\"match_type\":\"exact\",\"target\":[\"header_test\",\"field32\"]},{\"match_type\":\"valid\",\"target\":\"header_test_1\"}],\"actions\":[\"actionA\"],\"next_tables\":{\"actionA\":\"Learn\"},\"default_action\":null},{\"name\":\"Indirect\",\"id\":6,\"match_type\":\"exact\",\"type\":\"indirect\",\"act_prof_name\":\"ActProf\",\"max_size\":512,\"with_counters\":false,\"key\":[{\"match_type\":\"exact\",\"target\":[\"header_test\",\"field32\"]}],\"actions\":[\"actionA\",\"actionB\"],\"next_tables\":{\"actionA\":\"IndirectWS\",\"actionB\":\"IndirectWS\"},\"default_action\":null},{\"name\":\"IndirectWS\",\"id\":7,\"match_type\":\"exact\",\"type\":\"indirect_ws\",\"act_prof_name\":\"ActProfWS\",\"selector\":{\"algo\":\"crc16\",\"input\":[{\"type\":\"field\",\"value\":[\"header_test\",\"field24\"]},{\"type\":\"field\",\"value\":[\"header_test\",\"field48\"]},{\"type\":\"field\",\"value\":[\"header_test\",\"field64\"]}]},\"max_size\":512,\"with_counters\":false,\"key\":[{\"match_type\":\"exact\",\"target\":[\"header_test\",\"field32\"]}],\"actions\":[\"actionA\",\"actionB\"],\"next_tables\":{\"actionA\":null,\"actionB\":null},\"default_action\":null},{\"name\":\"Learn\",\"id\":8,\"match_type\":\"exact\",\"type\":\"simple\",\"max_size\":512,\"with_counters\":false,\"key\":[{\"match_type\":\"exact\",\"target\":[\"header_test\",\"field32\"]}],\"actions\":[\"ActionLearn\"],\"next_tables\":{\"ActionLearn\":\"Indirect\"},\"default_action\":null}],\"conditionals\":[]},{\"name\":\"egress\",\"id\":1,\"init_table\":null,\"tables\":[],\"conditionals\":[]}],\"calculations\":[{\"name\":\"SelectorHash\",\"id\":0,\"input\":[{\"type\":\"field\",\"value\":[\"header_test\",\"field24\"]},{\"type\":\"field\",\"value\":[\"header_test\",\"field48\"]},{\"type\":\"field\",\"value\":[\"header_test\",\"field64\"]}],\"algo\":\"crc16\"}],\"checksums\":[],\"learn_lists\":[{\"id\":1,\"name\":\"LearnDigest\",\"elements\":[{\"type\":\"field\",\"value\":[\"header_test\",\"field32\"]},{\"type\":\"field\",\"value\":[\"header_test\",\"field16\"]}]}]}";

TEST(P4Objects, LoadFromJSON1) {
  std::istringstream is(JSON_TEST_STRING_1);
  P4Objects objects;
  LookupStructureFactory factory;
  ASSERT_EQ(0, objects.init_objects(&is, &factory));

  ASSERT_NE(nullptr, objects.get_pipeline("ingress"));
  ASSERT_NE(nullptr, objects.get_action("ipv4_lpm", "_drop"));
  ASSERT_NE(nullptr, objects.get_parser("parser"));
  ASSERT_NE(nullptr, objects.get_deparser("deparser"));
  MatchTableAbstract *table;
  table = objects.get_abstract_match_table("forward");
  ASSERT_NE(nullptr, table);
  ASSERT_NE(nullptr, dynamic_cast<MatchTable *>(table));
  table = objects.get_abstract_match_table("ipv4_lpm");
  ASSERT_NE(nullptr, table);
  ASSERT_NE(nullptr, dynamic_cast<MatchTable *>(table));
  ASSERT_NE(nullptr, objects.get_match_action_table("forward"));
  ASSERT_NE(nullptr, objects.get_conditional("_condition_0"));
  ASSERT_NE(nullptr, objects.get_control_node("forward"));

  // objects.destroy_objects();
}

TEST(P4Objects, LoadFromJSON2) {
  std::istringstream is(JSON_TEST_STRING_2);
  P4Objects objects;
  LookupStructureFactory factory;
  ASSERT_EQ(0, objects.init_objects(&is, &factory));

  // this second test just checks that learn lists and indirect tables get
  // parsed correctly

  MatchTableAbstract *table;
  table = objects.get_abstract_match_table("Indirect");
  ASSERT_NE(nullptr, table);
  ASSERT_NE(nullptr, dynamic_cast<MatchTableIndirect *>(table));
  table = objects.get_abstract_match_table("IndirectWS");
  ASSERT_NE(nullptr, table);
  ASSERT_NE(nullptr, dynamic_cast<MatchTableIndirectWS *>(table));
}

TEST(P4Objects, Empty) {
  std::istringstream is("{}");
  P4Objects objects;
  LookupStructureFactory factory;
  ASSERT_EQ(0, objects.init_objects(&is, &factory));
}

TEST(P4Objects, UnknownPrimitive) {
  std::istringstream is("{\"actions\":[{\"name\":\"_drop\",\"id\":2,\"runtime_data\":[],\"primitives\":[{\"op\":\"bad_primitive\",\"parameters\":[]}]}]}");
  std::stringstream os;
  P4Objects objects(os);
  LookupStructureFactory factory;
  std::string expected("Unknown primitive action: bad_primitive\n");
  ASSERT_NE(0, objects.init_objects(&is, &factory));
  EXPECT_EQ(expected, os.str());
}

TEST(P4Objects, PrimitiveBadParamCount) {
  std::istringstream is("{\"actions\":[{\"name\":\"_drop\",\"id\":2,\"runtime_data\":[],\"primitives\":[{\"op\":\"drop\",\"parameters\":[{\"type\":\"hexstr\",\"value\":\"0xab\"}]}]}]}");
  std::stringstream os;
  LookupStructureFactory factory;
  P4Objects objects(os);
  std::string expected("Invalid number of parameters for primitive action drop: expected 0 but got 1\n");
  ASSERT_NE(0, objects.init_objects(&is, &factory));
  EXPECT_EQ(expected, os.str());
}

TEST(P4Objects, UnknownHash) {
  std::istringstream is("{\"calculations\":[{\"name\":\"calc\",\"id\":0,\"input\":[],\"algo\":\"bad_hash_1\"}]}");
  std::stringstream os;
  LookupStructureFactory factory;
  P4Objects objects(os);
  std::string expected("Unknown hash algorithm: bad_hash_1\n");
  ASSERT_NE(0, objects.init_objects(&is, &factory));
  EXPECT_EQ(expected, os.str());
}

TEST(P4Objects, UnknownHashSelector) {
  std::istringstream is("{\"pipelines\":[{\"name\":\"ingress\",\"id\":0,\"init_table\":\"t1\",\"tables\":[{\"name\":\"t1\",\"id\":0,\"match_type\":\"exact\",\"type\":\"indirect_ws\",\"selector\":{\"algo\":\"bad_hash_2\",\"input\":[]},\"max_size\":1024,\"with_counters\":false,\"key\":[],\"actions\":[\"_drop\"],\"next_tables\":{\"_drop\":null},\"default_action\":null}]}]}");
  std::stringstream os;
  LookupStructureFactory factory;
  P4Objects objects(os);
  std::string expected("Unknown hash algorithm: bad_hash_2\n");
  ASSERT_NE(0, objects.init_objects(&is, &factory));
  EXPECT_EQ(expected, os.str());
}

TEST(P4Objects, RequiredField) {
  std::istringstream is("{}");
  std::set<P4Objects::header_field_pair> required_fields;
  required_fields.insert(std::make_pair("standard_metadata", "egress_port"));
  std::stringstream os;
  LookupStructureFactory factory;
  P4Objects objects(os);
  std::string expected("Field standard_metadata.egress_port is required by switch target but is not defined\n");
  // 0 for device_id, 0 for cxt_id, nullptr for transport
  ASSERT_NE(0, objects.init_objects(&is, &factory, 0, 0, nullptr, required_fields));
  EXPECT_EQ(expected, os.str());
}

TEST(P4Objects, FieldAlias) {
  std::istringstream is("{\"header_types\":[{\"name\":\"hdrA_t\",\"id\":0,\"fields\":[[\"f1\",8],[\"f2\",8]]}],\"headers\":[{\"name\":\"hdrA\",\"id\":0,\"header_type\":\"hdrA_t\"}],\"field_aliases\":[[\"this_is.my_alias\",[\"hdrA\",\"f1\"]]]}");
  P4Objects objects;
  LookupStructureFactory factory;
  ASSERT_EQ(0, objects.init_objects(&is, &factory));

  ASSERT_TRUE(objects.field_exists("hdrA", "f1"));
  ASSERT_TRUE(objects.field_exists("this_is", "my_alias"));

  ASSERT_FALSE(objects.field_exists("hdrA", "fbad"));
  ASSERT_FALSE(objects.field_exists("hdrBad", "f1"));
  ASSERT_FALSE(objects.field_exists("this_is_not", "my_alias"));
  ASSERT_FALSE(objects.field_exists("this_is", "not_my_alias"));
}

TEST(P4Objects, Reset) {
  std::istringstream is(JSON_TEST_STRING_1);
  P4Objects objects;
  LookupStructureFactory factory;
  ASSERT_EQ(0, objects.init_objects(&is, &factory));
  // TODO(antonin): this test is not doing anything useful, but it is pretty
  // hard to test for reset
  objects.reset_state();
}

class my_extern_type : public ExternType {
 public:
  BM_EXTERN_ATTRIBUTES {
    BM_EXTERN_ATTRIBUTE_ADD(attr1);
  }

  void methodA() { }

  void init() override { }

 private:
  Data attr1{0};
};

BM_REGISTER_EXTERN(my_extern_type);
BM_REGISTER_EXTERN_METHOD(my_extern_type, methodA);

namespace {

void create_extern_instance_json(std::ostream *ss,
                                 const std::string &instance_name,
                                 const std::string &type_name,
                                 const std::string &attr_name,
                                 const std::string &attr_type) {
  *ss << "{\"extern_instances\":[{\"name\":\""
      << instance_name
      << "\",\"id\":22,\"type\":\""
      << type_name
      << "\",\"attribute_values\":[{\"name\":\""
      << attr_name
      << "\",\"type\":\""
      << attr_type
      << "\",\"value\":\"0xab\"}]}]}";
}

}  // namespace

TEST(P4Objects, ExternInstanceDeclaration) {
  std::stringstream is;

  {
    std::stringstream os;
    P4Objects objects(os);
    LookupStructureFactory factory;
    create_extern_instance_json(&is, "my_extern_instance", "my_extern_type",
                                "attr1", "hexstr");
    ASSERT_EQ(0, objects.init_objects(&is, &factory));
  }

  {
    std::stringstream os;
    P4Objects objects(os);
    LookupStructureFactory factory;
    create_extern_instance_json(&is, "my_extern_instance", "bad_type",
                                "attr1", "hexstr");
    std::string expected_error_msg = "Invalid reference to extern type 'bad_type'\n";
    ASSERT_NE(0, objects.init_objects(&is, &factory));
    EXPECT_EQ(expected_error_msg, os.str());
  }

  {
    std::stringstream os;
    P4Objects objects(os);
    LookupStructureFactory factory;
    create_extern_instance_json(&is, "my_extern_instance", "my_extern_type",
                                "bad_attr", "hexstr");
    std::string expected_error_msg = "Extern type 'my_extern_type' has no attribute 'bad_attr'\n";
    ASSERT_NE(0, objects.init_objects(&is, &factory));
    EXPECT_EQ(expected_error_msg, os.str());
  }

  {
    std::stringstream os;
    P4Objects objects(os);
    LookupStructureFactory factory;
    create_extern_instance_json(&is, "my_extern_instance", "my_extern_type",
                                "attr1", "unsupported_type");
    std::string expected_error_msg = "Only attributes of type 'hexstr' are supported for extern instance attribute initialization\n";
    ASSERT_NE(0, objects.init_objects(&is, &factory));
    EXPECT_EQ(expected_error_msg, os.str());
  }
}

TEST(P4Objects, TableDefaultEntry) {
  std::stringstream is;
  is << "{\"pipelines\":[{\"name\":\"ingress\",\"id\":0,\"init_table\":\"t0\","
     << "\"tables\":[{\"name\":\"t0\",\"id\":0,\"match_type\":\"exact\","
     << "\"type\":\"simple\",\"max_size\":1,\"with_counters\":false,"
     << "\"key\":[],\"action_ids\":[0],\"next_tables\":{\"a0\":null},"
     << "\"default_entry\":{\"action_id\":0,\"action_const\":true,"
     << "\"action_data\":[\"0xab\"],\"action_entry_const\":true}}]}],"
     << "\"actions\":[{\"name\":\"a0\",\"id\":0,\"runtime_data\":"
     << "[{\"name\":\"p\",\"bitwidth\":32}],\"primitives\":[]}]}";
  P4Objects objects;
  LookupStructureFactory factory;
  ASSERT_EQ(0, objects.init_objects(&is, &factory));
  auto t_ = objects.get_abstract_match_table("t0");
  auto t = dynamic_cast<MatchTable *>(t_);
  ASSERT_NE(nullptr, t);
  MatchTable::Entry entry;
  auto rc = t->get_default_entry(&entry);
  ASSERT_EQ(MatchErrorCode::SUCCESS, rc);
  ASSERT_EQ("a0", entry.action_fn->get_name());
  ASSERT_EQ(1u, entry.action_data.size());
  ASSERT_EQ(0xab, entry.action_data.get(0).get_int());
}

TEST(P4Objects, ParseVset) {
  fs::path json_path = fs::path(TESTDATADIR) / fs::path("parse_vset.json");
  std::ifstream is(json_path.string());
  P4Objects objects;
  LookupStructureFactory factory;
  ASSERT_EQ(0, objects.init_objects(&is, &factory));
  auto parse_vset_1 = objects.get_parse_vset("pv1");
  ASSERT_NE(nullptr, parse_vset_1);
  auto parse_vset_2 = objects.get_parse_vset("pv2");
  ASSERT_NE(nullptr, parse_vset_2);
  ASSERT_EQ("pv1", parse_vset_1->get_name());
  ASSERT_EQ(16, parse_vset_1->get_compressed_bitwidth());
}
