#include <core.p4>
#include <v1model.p4>

header h0_t {
    bit<8> v0;
    bit<24> v1;
    bit<32> v2;
}

header ha_t {
    bit<16> v0;
    bit<16> v1;
}

header hb_t {
    bit<16> v0;
    bit<16> v1;
}

header hc_t {
    bit<16> v0;
    bit<16> v1;
}

header sentinel_t {
    bit<32> val;
}

struct headers {
    h0_t h0;
    ha_t ha0;
    hb_t hb0;
    hc_t hc0;
    sentinel_t sentinel;
}

struct look_t {
    bit<1> a;
    bool b;
}

struct metadata_t {
}

control DeparserI(packet_out packet,
                  in headers hdr) {
    apply { packet.emit(hdr); }
}

parser parserI(packet_in pkt, out headers hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    look_t var_parser_a;

    state start {
        var_parser_a = pkt.lookahead<look_t>();
        pkt.extract(hdr.h0);
        transition select(var_parser_a.a) {
            1: parse_a;
            default: maybe_b;
        }
    }

    state parse_a {
        pkt.extract(hdr.ha0);
        transition maybe_b;
    }

    state maybe_b {
        transition select(var_parser_a.b) {
            true: parse_b;
            default: accept;
        }
    }

    state parse_b {
        pkt.extract(hdr.hb0);
        transition accept;
    }
}

control cIngress(inout headers hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    apply {
        hdr.sentinel.setValid();
        hdr.sentinel.val = 0b01010101_11001100_11110000_11111111;
    }
}

control cEgress(inout headers hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    apply { }
}

control vc(inout headers hdr, inout metadata_t meta) {
    apply { }
}

control uc(inout headers hdr, inout metadata_t meta) {
    apply { }
}

V1Switch<headers, metadata_t>(parserI(), vc(), cIngress(), cEgress(), uc(), DeparserI()) main;
