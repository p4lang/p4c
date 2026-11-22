#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

struct headers_t {
}

struct metadata_t {
}

parser parserImpl(packet_in packet, out headers_t hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    state start {
        transition accept;
    }
}

control verifyChecksum(inout headers_t hdr, inout metadata_t meta) {
    apply {
    }
}

control ingressImpl(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    action my_drop() {
        mark_to_drop(stdmeta);
    }
    action redirect() {
        stdmeta.egress_spec = stdmeta.ingress_port;
    }
    @MixedExprList[1, "hello", true, false, 11] @MixedKV[label="text", my_bool=true, int_val=6] table t {
        actions = {
            @myid[1] redirect();
            @myid[2] my_drop();
            NoAction();
        }
        default_action = NoAction();
    }
    apply {
        t.apply();
    }
}

control egressImpl(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    apply {
    }
}

control updateChecksum(inout headers_t hdr, inout metadata_t meta) {
    apply {
    }
}

control deparserImpl(packet_out packet, in headers_t hdr) {
    apply {
        packet.emit<headers_t>(hdr);
    }
}

V1Switch<headers_t, metadata_t>(parserImpl(), verifyChecksum(), ingressImpl(), egressImpl(), updateChecksum(), deparserImpl()) main;
