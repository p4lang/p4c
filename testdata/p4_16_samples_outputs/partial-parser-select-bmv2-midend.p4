#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header dispatch_t {
    bit<8> kind;
}

struct headers_t {
    dispatch_t dispatch;
}

struct metadata_t {
    bit<9> port;
}

control VerifyChecksumImpl(inout headers_t hdr, inout metadata_t meta) {
    apply {
    }
}

control IngressImpl(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t sm) {
    @hidden action partialparserselectbmv2l52() {
        mark_to_drop(sm);
    }
    @hidden action partialparserselectbmv2l54() {
        sm.egress_spec = meta.port;
    }
    @hidden table tbl_partialparserselectbmv2l52 {
        actions = {
            partialparserselectbmv2l52();
        }
        const default_action = partialparserselectbmv2l52();
    }
    @hidden table tbl_partialparserselectbmv2l54 {
        actions = {
            partialparserselectbmv2l54();
        }
        const default_action = partialparserselectbmv2l54();
    }
    apply {
        if (sm.parser_error != error.NoError) {
            tbl_partialparserselectbmv2l52.apply();
        } else {
            tbl_partialparserselectbmv2l54.apply();
        }
    }
}

control EgressImpl(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t sm) {
    apply {
    }
}

control ComputeChecksumImpl(inout headers_t hdr, inout metadata_t meta) {
    apply {
    }
}

control DeparserImpl(packet_out packet, in headers_t hdr) {
    apply {
        packet.emit<dispatch_t>(hdr.dispatch);
    }
}

parser ParserImpl_0(packet_in packet, out headers_t hdr, inout metadata_t meta, inout standard_metadata_t sm) {
    state start {
        packet.extract<dispatch_t>(hdr.dispatch);
        transition select(hdr.dispatch.kind) {
            8w0x10 &&& 8w0xf0: masked;
            8w0x10: shadowed;
            8w0x20 &&& 8w0xf0: ranged;
            8w0xfe: late_wildcard;
            8w0xff: nested_dispatch;
            default: other;
        }
    }
    state masked {
        meta.port = 9w3;
        transition accept;
    }
    state shadowed {
        meta.port = 9w4;
        transition accept;
    }
    state ranged {
        meta.port = 9w5;
        transition accept;
    }
    state other {
        meta.port = 9w6;
        transition accept;
    }
    state nested_dispatch {
        transition select(hdr.dispatch.kind, hdr.dispatch.kind) {
            (8w0xff, 8w0xff): masked;
            default: other;
        }
    }
    state late_wildcard {
        transition select(hdr.dispatch.kind) {
            default: masked;
        }
    }
}

V1Switch<headers_t, metadata_t>(ParserImpl_0(), VerifyChecksumImpl(), IngressImpl(), EgressImpl(), ComputeChecksumImpl(), DeparserImpl()) main;
