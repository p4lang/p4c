#include <gtest/gtest.h>

#include <sstream>
#include <string>

#include "bm_sim/P4Objects.h"

int pull_test_p4objects() { return 0; }

class modify_field : public ActionPrimitive<Field &, const Data &> {
  void operator ()(Field &f, const Data &d) {
    f.set(d);
  }
};

REGISTER_PRIMITIVE(modify_field);

class drop : public ActionPrimitive<> {
  void operator ()() {

  }
};

REGISTER_PRIMITIVE(drop);

class add_to_field : public ActionPrimitive<Field &, const Data &> {
  void operator ()(Field &f, const Data &d) {
    f.add(f, d);
  }
};

REGISTER_PRIMITIVE(add_to_field);

const std::string JSON_TEST_STRING =
  "{\"header_types\":[{\"name\":\"standard_metadata_t\",\"id\":0,\"fields\":[[\"ingress_port\",9],[\"packet_length\",32],[\"egress_spec\",9],[\"egress_port\",9],[\"egress_instance\",32],[\"instance_type\",32],[\"clone_spec\",32],[\"_padding\",5]]},{\"name\":\"ethernet_t\",\"id\":1,\"fields\":[[\"dstAddr\",48],[\"srcAddr\",48],[\"etherType\",16]]},{\"name\":\"ipv4_t\",\"id\":2,\"fields\":[[\"version\",4],[\"ihl\",4],[\"diffserv\",8],[\"totalLen\",16],[\"identification\",16],[\"flags\",3],[\"fragOffset\",13],[\"ttl\",8],[\"protocol\",8],[\"hdrChecksum\",16],[\"srcAddr\",32],[\"dstAddr\",32]]},{\"name\":\"routing_metadata_t\",\"id\":3,\"fields\":[[\"nhop_ipv4\",32]]}],\"headers\":[{\"name\":\"standard_metadata\",\"id\":0,\"header_type\":\"standard_metadata_t\"},{\"name\":\"ethernet\",\"id\":1,\"header_type\":\"ethernet_t\"},{\"name\":\"ipv4\",\"id\":2,\"header_type\":\"ipv4_t\"},{\"name\":\"routing_metadata\",\"id\":3,\"header_type\":\"routing_metadata_t\"}],\"parsers\":[{\"name\":\"parser\",\"id\":0,\"init_state\":\"start\",\"parse_states\":[{\"name\":\"start\",\"id\":0,\"extracts\":[],\"sets\":[],\"transition_key\":[],\"transitions\":[{\"value\":\"default\",\"mask\":null,\"next_state\":\"parse_ethernet\"}]},{\"name\":\"parse_ethernet\",\"id\":1,\"extracts\":[\"ethernet\"],\"sets\":[],\"transition_key\":[[\"ethernet\",\"etherType\"]],\"transitions\":[{\"value\":\"0x800\",\"mask\":null,\"next_state\":\"parse_ipv4\"},{\"value\":\"default\",\"mask\":null,\"next_state\":null}]},{\"name\":\"parse_ipv4\",\"id\":2,\"extracts\":[\"ipv4\"],\"sets\":[],\"transition_key\":[],\"transitions\":[{\"value\":\"default\",\"mask\":null,\"next_state\":null}]}]}],\"deparsers\":[{\"name\":\"deparser\",\"id\":0,\"order\":[\"ethernet\",\"ipv4\"]}],\"actions\":[{\"name\":\"rewrite_mac\",\"id\":0,\"runtime_data\":[{\"name\":\"smac\",\"bitwidth\":48}],\"primitives\":[{\"op\":\"modify_field\",\"parameters\":[{\"type\":\"field\",\"value\":[\"ethernet\",\"srcAddr\"]},{\"type\":\"runtime_data\",\"value\":0}]}]},{\"name\":\"set_nhop\",\"id\":1,\"runtime_data\":[{\"name\":\"nhop_ipv4\",\"bitwidth\":32},{\"name\":\"port\",\"bitwidth\":9}],\"primitives\":[{\"op\":\"modify_field\",\"parameters\":[{\"type\":\"field\",\"value\":[\"routing_metadata\",\"nhop_ipv4\"]},{\"type\":\"runtime_data\",\"value\":0}]},{\"op\":\"modify_field\",\"parameters\":[{\"type\":\"field\",\"value\":[\"standard_metadata\",\"egress_spec\"]},{\"type\":\"runtime_data\",\"value\":1}]},{\"op\":\"add_to_field\",\"parameters\":[{\"type\":\"field\",\"value\":[\"ipv4\",\"ttl\"]},{\"type\":\"hexstr\",\"value\":\"-0x1\"}]}]},{\"name\":\"_drop\",\"id\":2,\"runtime_data\":[],\"primitives\":[{\"op\":\"drop\",\"parameters\":[]}]},{\"name\":\"set_dmac\",\"id\":3,\"runtime_data\":[{\"name\":\"dmac\",\"bitwidth\":48}],\"primitives\":[{\"op\":\"modify_field\",\"parameters\":[{\"type\":\"field\",\"value\":[\"ethernet\",\"dstAddr\"]},{\"type\":\"runtime_data\",\"value\":0}]}]}],\"pipelines\":[{\"name\":\"ingress\",\"id\":0,\"init_table\":\"_condition_0\",\"tables\":[{\"name\":\"ipv4_lpm\",\"id\":0,\"type\":\"lpm\",\"max_size\":1024,\"key\":[{\"match_type\":\"lpm\",\"target\":[\"ipv4\",\"dstAddr\"]}],\"actions\":[\"set_nhop\",\"_drop\"],\"next_tables\":{\"set_nhop\":\"forward\",\"_drop\":\"forward\"},\"default_action\":null},{\"name\":\"forward\",\"id\":1,\"type\":\"exact\",\"max_size\":512,\"key\":[{\"match_type\":\"exact\",\"target\":[\"routing_metadata\",\"nhop_ipv4\"]}],\"actions\":[\"set_dmac\",\"_drop\"],\"next_tables\":{\"set_dmac\":null,\"_drop\":null},\"default_action\":null}],\"conditionals\":[{\"name\":\"_condition_0\",\"id\":0,\"expression\":{\"type\":\"expression\",\"value\":{\"op\":\"and\",\"left\":{\"type\":\"expression\",\"value\":{\"op\":\"valid\",\"left\":null,\"right\":{\"type\":\"header\",\"value\":\"ipv4\"}}},\"right\":{\"type\":\"expression\",\"value\":{\"op\":\">\",\"left\":{\"type\":\"field\",\"value\":[\"ipv4\",\"ttl\"]},\"right\":{\"type\":\"hexstr\",\"value\":\"0x0\"}}}}},\"true_next\":\"ipv4_lpm\",\"false_next\":null}]},{\"name\":\"egress\",\"id\":1,\"init_table\":\"send_frame\",\"tables\":[{\"name\":\"send_frame\",\"id\":2,\"type\":\"exact\",\"max_size\":256,\"key\":[{\"match_type\":\"exact\",\"target\":[\"standard_metadata\",\"egress_port\"]}],\"actions\":[\"rewrite_mac\",\"_drop\"],\"next_tables\":{\"rewrite_mac\":null,\"_drop\":null},\"default_action\":null}],\"conditionals\":[]}]}";

TEST(P4Objects, LoadFromJSON) {
  std::stringstream is(JSON_TEST_STRING);
  P4Objects objects;
  objects.init_objects(is);

  ASSERT_NE(nullptr, objects.get_pipeline("ingress"));
  ASSERT_NE(nullptr, objects.get_action("_drop"));
  ASSERT_NE(nullptr, objects.get_parser("parser"));
  ASSERT_NE(nullptr, objects.get_deparser("deparser"));
  ASSERT_NE(nullptr, objects.get_exact_match_table("forward"));
  ASSERT_NE(nullptr, objects.get_lpm_table("ipv4_lpm"));
  ASSERT_NE(nullptr, objects.get_match_table("forward"));
  ASSERT_NE(nullptr, objects.get_conditional("_condition_0"));
  ASSERT_NE(nullptr, objects.get_control_node("forward"));

  objects.destroy_objects();
}
