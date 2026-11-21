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

parser ParserImpl(packet_in packet, out headers_t hdr, inout metadata_t meta, inout standard_metadata_t sm)(bit<16> profile) {
    bool enabled;
    state start {
        packet.extract<dispatch_t>(hdr.dispatch);
        transition select(profile, hdr.dispatch.kind) {
            (16w2, 8w0x10): disabled;
            (16w1, 8w0x10 &&& 8w0xf0): masked;
            (16w1, 8w0x10): shadowed;
            (16w1, 8w0x20 .. 8w0x2f): ranged;
            (16w1, 8w0xfe): late_wildcard;
            (16w1, 8w0xff): nested_dispatch;
            default: other;
        }
    }
    state disabled {
        meta.port = 9w2;
        transition accept;
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
        transition select(profile, { hdr.dispatch.kind, hdr.dispatch.kind }) {
            (16w1, { 8w0xff, 8w0xff }): masked;
            default: other;
        }
    }
    state late_wildcard {
        enabled = true;
        transition select(hdr.dispatch.kind, enabled) {
            (default, true): masked;
            default: other;
        }
    }
}

control VerifyChecksumImpl(inout headers_t hdr, inout metadata_t meta) {
    apply {
    }
}

control IngressImpl(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t sm) {
    apply {
        if (sm.parser_error != error.NoError) {
            mark_to_drop(sm);
        } else {
            sm.egress_spec = meta.port;
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

V1Switch<headers_t, metadata_t>(ParserImpl(16w1), VerifyChecksumImpl(), IngressImpl(), EgressImpl(), ComputeChecksumImpl(), DeparserImpl()) main;
