#include <gtest/gtest.h>

#include <sstream>
#include <string>

#include "P4Objects.h"

int pull_test_p4objects() { return 0; }

const std::string JSON_TEST_STRING =
  "{\"header_types\":[{\"name\":\"ethernet_t\",\"fields\":{\"dstAddr\":48,\"srcAddr\":48,\"ethertype\":16}},{\"name\":\"ipv4_t\",\"fields\":{\"version\":4,\"ihl\":4,\"diffserv\":8,\"len\":16,\"id\":16,\"flags\":3,\"flagOffset\":13,\"ttl\":8,\"protocol\":8,\"checksum\":16,\"srcAddr\":32,\"dstAddr\":32}},{\"name\":\"intrinsic_metadata_t\",\"fields\":{\"drop_signal\":1,\"egress_port\":9,\"_padding\":6}}],\"headers\":[{\"name\":\"ethernet\",\"header_type\":\"ethernet_t\"},{\"name\":\"ipv4\",\"header_type\":\"ipv4_t\"},{\"name\":\"intrinsic_metadata\",\"header_type\":\"intrinsic_metadata_t\"}],\"parsers\":[{\"name\":\"parser1\",\"init_state\":\"parse_ethernet\",\"parse_states\":[{\"name\":\"parse_ethernet\",\"extracts\":[\"ethernet\"],\"sets\":[],\"transition_key\":[[\"ethernet\",\"ethertype\"]],\"transitions\":[{\"value\":\"0x0800\",\"mask\":\"0xFFFF\",\"next_state\":\"parse_ipv4\"}]},{\"name\":\"parse_ipv4\",\"extracts\":[\"ipv4\"],\"sets\":[],\"transition_key\":[],\"transitions\":[]}]}],\"deparsers\":[{\"name\":\"deparser1\",\"order\":[\"ethernet\",\"ipv4\"]}],\"pipelines\":[{\"name\":\"pipeline1\",\"tables\":[{\"name\":\"switch\",\"type\":\"exact\",\"max_size\":1024,\"key\":[[\"ethernet\",\"dstAddr\"]],\"actions\":[\"drop\",\"forward\"],\"next_tables\":{\"drop\":null,\"forward\":null},\"default_action\":null,\"ageing\":false,\"entry_counters\":false}]}],\"actions\":[{\"name\":\"drop\",\"parameters\":[{\"type\":\"field\",\"value\":[\"intrinsic_metadata\",\"drop_signal\"]},{\"type\":\"immediate\",\"value\":\"0x1\"}],\"primitives\":[\"WRITE_FIELD\"]},{\"name\":\"forward\",\"parameters\":[{\"type\":\"field\",\"value\":[\"intrinsic_metadata\",\"egress_port\"]},{\"type\":\"runtime_data\",\"name\":\"port\",\"bitwidth\":9}],\"primitives\":[\"WRITE_FIELD\"]}],\"primitive_opcodes\":{\"WRITE_FIELD\":1}}";

TEST(P4Objects, LoadFromJSON) {
  std::stringstream is(JSON_TEST_STRING);
  P4Objects objects;
  objects.init_objects(is);

  ASSERT_TRUE(true);
  objects.destroy_objects();
}
