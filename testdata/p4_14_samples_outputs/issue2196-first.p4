#include <core.p4>
#define V1MODEL_VERSION 20200408
#include <v1model.p4>

struct my_meta_t {
    bit<8>  pv_parsed;
    bit<16> f1_16;
}

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> ethertype;
}

header my_header_t {
    bit<3>  f1;
    bit<13> f2;
}

struct metadata {
    @name(".my_meta")
    my_meta_t my_meta;
}

struct headers {
    @name(".ethernet")
    ethernet_t  ethernet;
    @name(".my_header")
    my_header_t my_header;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".pv2") value_set<bit<16>>(4) pv2;
    @name(".pv1") value_set<bit<16>>(4) pv1;
    @name(".parse_pv1") state parse_pv1 {
        meta.my_meta.pv_parsed = 8w1;
        packet.extract<my_header_t>(hdr.my_header);
        meta.my_meta.f1_16 = (bit<16>)hdr.my_header.f1;
        transition select(meta.my_meta.f1_16 << 13 | (bit<16>)hdr.my_header.f2) {
            pv2: parse_pv2;
            default: accept;
        }
    }
    @name(".parse_pv2") state parse_pv2 {
        meta.my_meta.pv_parsed = 8w2;
        transition accept;
    }
    @name(".start") state start {
        packet.extract<ethernet_t>(hdr.ethernet);
        transition select(hdr.ethernet.ethertype) {
            16w0x800: accept;
            pv1: parse_pv1;
            pv2: parse_pv2;
            default: accept;
        }
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".route_eth") action route_eth(bit<9> egress_spec, bit<48> src_addr) {
        standard_metadata.egress_spec = egress_spec;
        hdr.ethernet.src_addr = src_addr;
    }
    @name(".noop") action noop() {
    }
    @name(".routing") table routing {
        actions = {
            route_eth();
            noop();
            @defaultonly NoAction();
        }
        key = {
            hdr.ethernet.dst_addr: lpm @name("ethernet.dst_addr");
        }
        default_action = NoAction();
    }
    apply {
        routing.apply();
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit<ethernet_t>(hdr.ethernet);
        packet.emit<my_header_t>(hdr.my_header);
    }
}

control verifyChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

V1Switch<headers, metadata>(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;
