#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header h_t {
    bit<8> f;
}

struct metadata_t {
    bit<8> x;
}

struct headers_t {
    h_t h;
}

parser p(packet_in pkt, out headers_t hdr, inout metadata_t meta, inout standard_metadata_t std_meta) {
    state start {
        pkt.extract<h_t>(hdr.h);
        meta.x = hdr.h.f;
        transition accept;
    }
}

control ingress(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t std_meta) {
    @name("ingress.data") bit<8> data_0;
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("ingress.a") action a(@name("data") bit<8> data_1) {
        hdr.h.f = data_1;
    }
    @name("ingress.a") action a_1() {
        data_0 = meta.x;
        hdr.h.f = data_0;
    }
    @name("ingress.t") table t_0 {
        key = {
            hdr.h.f: exact @name("hdr.h.f");
        }
        actions = {
            a();
            @defaultonly NoAction_1();
        }
        default_action = NoAction_1();
    }
    apply {
        a_1();
        t_0.apply();
    }
}

control egress(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t std_meta) {
    apply {
    }
}

control deparser(packet_out pkt, in headers_t hdr) {
    apply {
        pkt.emit<h_t>(hdr.h);
    }
}

control verifyChecksum(inout headers_t hdr, inout metadata_t meta) {
    apply {
    }
}

control computeChecksum(inout headers_t hdr, inout metadata_t meta) {
    apply {
    }
}

V1Switch<headers_t, metadata_t>(p(), verifyChecksum(), ingress(), egress(), computeChecksum(), deparser()) main;
