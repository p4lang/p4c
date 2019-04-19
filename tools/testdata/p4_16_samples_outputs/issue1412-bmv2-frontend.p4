#include <core.p4>
#include <v1model.p4>

struct headers_t {
}

struct metadata {
    bool   cond;
    bit<8> field;
}

parser ParserImpl(packet_in buffer, out headers_t parsed_hdr, inout metadata meta, inout standard_metadata_t ostd) {
    state start {
        transition accept;
    }
}

control VerifyChecksumImpl(inout headers_t hdr, inout metadata meta) {
    apply {
    }
}

control IngressImpl(inout headers_t hdr, inout metadata meta, inout standard_metadata_t ostd) {
    apply {
    }
}

control EgressImpl(inout headers_t hdr, inout metadata meta, inout standard_metadata_t ostd) {
    @name(".NoAction") action NoAction_0() {
    }
    @name("EgressImpl.set_true") action set_true() {
        if (meta.field == 8w0) 
            meta.cond = true;
    }
    @name("EgressImpl.change_cond") table change_cond_0 {
        key = {
            ostd.egress_spec: exact @name("ostd.egress_spec") ;
        }
        actions = {
            set_true();
            @defaultonly NoAction_0();
        }
        default_action = NoAction_0();
    }
    apply {
        change_cond_0.apply();
    }
}

control ComputeChecksumImpl(inout headers_t hdr, inout metadata meta) {
    apply {
    }
}

control DeparserImpl(packet_out buffer, in headers_t hdr) {
    apply {
    }
}

V1Switch<headers_t, metadata>(ParserImpl(), VerifyChecksumImpl(), IngressImpl(), EgressImpl(), ComputeChecksumImpl(), DeparserImpl()) main;

