#include <core.p4>
#include <v1model.p4>

#define ETHERTYPE_IPV4 0x0800
#define IP_PROTOCOLS_TCP 6
#define IP_PROTOCOLS_UDP 17

serializable struct switch_metadata_t {
    bit<9> port;
    bit<10> session_id;
    bit<1> check;
    bit<4> pad;
}

serializable struct bridge_metadata_t {
    bit<9> port;
    bit<1> check;
    bit<6> pad;
}

typedef bit<16> switch_nexthop_t;

header serialized_bridge_metadata_t {
    bit<8> type;
    bridge_metadata_t meta;
}

header serialized_switch_metadata_t {
    bit<8> type;
    switch_metadata_t meta;
}

struct parsed_packet_t {
    serialized_switch_metadata_t mirrored_md;
}

struct local_metadata_t {
    serialized_bridge_metadata_t bridged_md;
}

parser parse(packet_in pk, out parsed_packet_t h, inout local_metadata_t local_metadata,
             inout standard_metadata_t standard_metadata) {
    state start {
	transition accept;
    }
}

control ingress(inout parsed_packet_t h, inout local_metadata_t local_metadata,
                inout standard_metadata_t standard_metadata) {
    apply {
	h.mirrored_md.setValid();
	h.mirrored_md.type = 8w0;
	h.mirrored_md.meta = { 9w0, 10w0, 1w0, 4w0 };
        clone3(CloneType.I2E, 0, h.mirrored_md);
    }
}

control egress(inout parsed_packet_t hdr, inout local_metadata_t local_metadata,
               inout standard_metadata_t standard_metadata) {
    apply { }
}

control deparser(packet_out b, in parsed_packet_t h) {
    apply { }
}

control verify_checksum(inout parsed_packet_t hdr,
inout local_metadata_t local_metadata) {
    apply { }
}

control compute_checksum(inout parsed_packet_t hdr,
                         inout local_metadata_t local_metadata) {
    apply { }
}

V1Switch(parse(), verify_checksum(), ingress(), egress(),
compute_checksum(), deparser()) main;
