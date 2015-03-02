#include <gtest/gtest.h>

#include <sstream>
#include <string>

#include "behavioral_sim/P4Objects.h"

int pull_test_p4objects() { return 0; }

const std::string JSON_TEST_STRING =
  "{\"header_types\":[{\"name\":\"ethernet_t\",\"fields\":{\"dstAddr\":48,\"srcAddr\":48,\"ethertype\":16}},{\"name\":\"ipv4_t\",\"fields\":{\"version\":4,\"ihl\":4,\"diffserv\":8,\"len\":16,\"id\":16,\"flags\":3,\"flagOffset\":13,\"ttl\":8,\"protocol\":8,\"checksum\":16,\"srcAddr\":32,\"dstAddr\":32}},{\"name\":\"intrinsic_metadata_t\",\"fields\":{\"drop_signal\":1,\"egress_port\":9,\"_padding\":6}}],\"headers\":[{\"name\":\"ethernet\",\"header_type\":\"ethernet_t\"},{\"name\":\"ipv4\",\"header_type\":\"ipv4_t\"},{\"name\":\"intrinsic_metadata\",\"header_type\":\"intrinsic_metadata_t\"}],\"parsers\":[{\"name\":\"parser1\",\"init_state\":\"parse_ethernet\",\"parse_states\":[{\"name\":\"parse_ethernet\",\"extracts\":[\"ethernet\"],\"sets\":[],\"transition_key\":[[\"ethernet\",\"ethertype\"]],\"transitions\":[{\"value\":\"0x0800\",\"mask\":\"0xFFFF\",\"next_state\":\"parse_ipv4\"}]},{\"name\":\"parse_ipv4\",\"extracts\":[\"ipv4\"],\"sets\":[],\"transition_key\":[],\"transitions\":[]}]}],\"deparsers\":[{\"name\":\"deparser1\",\"order\":[\"ethernet\",\"ipv4\"]}],\"actions\":[{\"name\":\"drop\",\"runtime_data\":[],\"primitives\":[{\"op\":\"SetField\",\"parameters\":[{\"type\":\"field\",\"value\":[\"intrinsic_metadata\",\"drop_signal\"]},{\"type\":\"hexstr\",\"value\":\"0x1\"}]}]},{\"name\":\"forward\",\"runtime_data\":[{\"name\":\"port\",\"bitwidth\":9}],\"primitives\":[{\"op\":\"SetField\",\"parameters\":[{\"type\":\"field\",\"value\":[\"intrinsic_metadata\",\"egress_port\"]},{\"type\":\"runtime_data\",\"value\":0}]}]}],\"pipelines\":[{\"name\":\"pipeline1\",\"init_table\":\"switch\",\"tables\":[{\"name\":\"switch\",\"type\":\"exact\",\"max_size\":1024,\"key\":[{\"match_type\":\"exact\",\"target\":[\"ethernet\",\"dstAddr\"]}],\"actions\":[\"drop\",\"forward\"],\"next_tables\":{\"drop\":null,\"forward\":null},\"default_action\":null,\"ageing\":false,\"entry_counters\":false}],\"conditionals\":[]}]}";

TEST(P4Objects, LoadFromJSON) {
  std::stringstream is(JSON_TEST_STRING);
  P4Objects objects;
  objects.init_objects(is);

  // TODO: add actual tests ?

  ASSERT_NE(nullptr, objects.get_pipeline("pipeline1"));

  objects.destroy_objects();
}
