#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header ethernet_t {
    bit<48> da;
    bit<48> sa;
    bit<16> type;
}

header ipv4_t {
    bit<4>  version;
    bit<4>  ihl;
    bit<6>  dscp;
    bit<2>  ecn;
    bit<16> total_len;
    bit<16> identification;
    bit<1>  reserved;
    bit<1>  do_not_fragment;
    bit<1>  more_fragments;
    bit<13> frag_offset;
    bit<8>  ttl;
    bit<8>  protocol;
    bit<16> header_checksum;
    bit<32> src_addr;
    bit<32> dst_addr;
}

header ipv4_options_t {
    varbit<480> options;
}

struct headers_t {
    ethernet_t     mac;
    ipv4_options_t ip;
}

struct local_metadata_t {
}

parser v1model_parser(packet_in pkt, out headers_t hdrs, inout local_metadata_t local_meta, inout standard_metadata_t standard_meta) {
    @name("v1model_parser.ipv4") ipv4_t ipv4_0;
    bit<160> tmp;
    state start {
        pkt.extract<ethernet_t>(hdrs.mac);
        transition select(hdrs.mac.type) {
            16w0x800: parse_ipv4;
            default: accept;
        }
    }
    state parse_ipv4 {
        tmp = pkt.lookahead<bit<160>>();
        ipv4_0.setValid();
        ipv4_0.version = tmp[159:156];
        ipv4_0.ihl = tmp[155:152];
        ipv4_0.dscp = tmp[151:146];
        ipv4_0.ecn = tmp[145:144];
        ipv4_0.total_len = tmp[143:128];
        ipv4_0.identification = tmp[127:112];
        ipv4_0.reserved = tmp[111:111];
        ipv4_0.do_not_fragment = tmp[110:110];
        ipv4_0.more_fragments = tmp[109:109];
        ipv4_0.frag_offset = tmp[108:96];
        ipv4_0.ttl = tmp[95:88];
        ipv4_0.protocol = tmp[87:80];
        ipv4_0.header_checksum = tmp[79:64];
        ipv4_0.src_addr = tmp[63:32];
        ipv4_0.dst_addr = tmp[31:0];
        pkt.extract<ipv4_options_t>(hdrs.ip, (bit<32>)tmp[155:152] << 2 + 3);
        transition accept;
    }
}

control compute_ip_checksum(inout headers_t headers, inout local_metadata_t local_metadata) {
    apply {
    }
}

control verify_ip_checksum(inout headers_t headers, inout local_metadata_t local_metadata) {
    apply {
    }
}

control egress(inout headers_t headers, inout local_metadata_t local_metadata, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control ingress(inout headers_t headers, inout local_metadata_t local_metadata, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control deparser(packet_out packet, in headers_t headers) {
    apply {
    }
}

V1Switch<headers_t, local_metadata_t>(v1model_parser(), verify_ip_checksum(), ingress(), egress(), compute_ip_checksum(), deparser()) main;
