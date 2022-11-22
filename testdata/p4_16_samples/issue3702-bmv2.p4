//#include "third_party/p4lang_p4c/p4include/core.p4"
//#include "third_party/p4lang_p4c/p4include/v1model.p4"
#include "core.p4"
#include "v1model.p4"

typedef bit<48> ethernet_addr_t;
typedef bit<32> ipv4_addr_t;

// Enums
enum bit <16> EtherType {
ETYPE_IPV4 = 0x0800
};

// Header definitions
header ethernet_t {
ethernet_addr_t da; // destination address
ethernet_addr_t sa; // source address
bit<16> type; // First (mandatory) Ethertype field
}

header ipv4_t {
bit<4> version;
bit<4> ihl;
bit<6> dscp; // The 6 most significant bits of the diff_serv field.
bit<2> ecn; // The 2 least significant bits of the diff_serv field.
bit<16> total_len;
bit<16> identification;
bit<1> reserved;
bit<1> do_not_fragment;
bit<1> more_fragments;
bit<13> frag_offset;
bit<8> ttl;
bit<8> protocol;
bit<16> header_checksum;
ipv4_addr_t src_addr;
ipv4_addr_t dst_addr;
}

header ipv4_options_t {
varbit<480> options;
}

// Set of extracted headers.
struct headers_t {
ethernet_t mac; // mac header
ipv4_options_t ip; // v4/v6 union
}

// Required metadata structure.
struct local_metadata_t {

}

// Signature is governed by the architecture (PNA)
parser v1model_parser(packet_in pkt, out headers_t hdrs,
inout local_metadata_t local_meta,
inout standard_metadata_t standard_meta) {
state start {
pkt.extract(hdrs.mac);
transition select(hdrs.mac.type) {
EtherType.ETYPE_IPV4 : parse_ipv4;
default: accept;
}
}

state parse_ipv4 {
ipv4_t ipv4 = pkt.lookahead<ipv4_t>();
bit<32> byte_len = ((bit<32>)(ipv4.ihl & 0xF)) << 2;
pkt.extract(hdrs.ip, byte_len << 3);
transition accept;
}

}

control compute_ip_checksum(inout headers_t headers,
inout local_metadata_t local_metadata) {
apply {
}
}
control verify_ip_checksum(inout headers_t headers,
inout local_metadata_t local_metadata) {
apply {
}
}

control egress(inout headers_t headers,
inout local_metadata_t local_metadata,
inout standard_metadata_t standard_metadata) {
apply {
}
}
control ingress(inout headers_t headers,
inout local_metadata_t local_metadata,
inout standard_metadata_t standard_metadata) {
apply {
}
}

control deparser(packet_out packet, in headers_t headers) {
apply {
}
}

// Main invocation
V1Switch(v1model_parser(), verify_ip_checksum(), ingress(), egress(), compute_ip_checksum(), deparser()) main;
