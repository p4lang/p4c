#include <core.p4>
#include <v1model.p4>

struct PortId_t {
    bit<9> _v;
}

struct parsed_headers_t {
}

struct metadata_t {
    bit<9> _foo__v0;
    bit<9> _bar__v1;
}

parser ParserImpl(packet_in packet, out parsed_headers_t hdr, inout metadata_t meta, inout standard_metadata_t standard_metadata) {
    state start {
        transition accept;
    }
}

control IngressImpl(inout parsed_headers_t hdr, inout metadata_t meta, inout standard_metadata_t standard_metadata) {
    @hidden action act() {
        meta._foo__v0 = meta._foo__v0 + 9w1;
    }
    @hidden action act_0() {
        meta._foo__v0 = meta._foo__v0 + 9w1;
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    @hidden table tbl_act_0 {
        actions = {
            act_0();
        }
        const default_action = act_0();
    }
    apply {
        if (meta._foo__v0 == meta._bar__v1) 
            tbl_act.apply();
        if (meta._foo__v0 == 9w192) 
            tbl_act_0.apply();
    }
}

control EgressImpl(inout parsed_headers_t hdr, inout metadata_t meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control DeparserImpl(packet_out packet, in parsed_headers_t hdr) {
    apply {
    }
}

control VerifyChecksumImpl(inout parsed_headers_t hdr, inout metadata_t meta) {
    apply {
    }
}

control ComputeChecksumImpl(inout parsed_headers_t hdr, inout metadata_t meta) {
    apply {
    }
}

V1Switch<parsed_headers_t, metadata_t>(ParserImpl(), VerifyChecksumImpl(), IngressImpl(), EgressImpl(), ComputeChecksumImpl(), DeparserImpl()) main;

