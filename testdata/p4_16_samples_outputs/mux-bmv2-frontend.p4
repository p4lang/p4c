#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

struct Headers {
}

struct Metadata {
}

parser P(packet_in b, out Headers p, inout Metadata meta, inout standard_metadata_t standard_meta) {
    state start {
        transition accept;
    }
}

control Ing(inout Headers headers, inout Metadata meta, inout standard_metadata_t standard_meta) {
    apply {
    }
}

control Eg(inout Headers hdrs, inout Metadata meta, inout standard_metadata_t standard_meta) {
    @name("Eg._sub") bit<32> _sub_0;
    @name("Eg.res") bit<64> res_0;
    @name("Eg.tmp") bit<32> tmp;
    @name("Eg.p") bool p_0;
    @name("Eg.val") bit<64> val_0;
    @name("Eg.update") action update() {
        p_0 = true;
        val_0 = res_0;
        _sub_0 = val_0[31:0];
        if (p_0) {
            tmp = _sub_0;
        } else {
            tmp = 32w1;
        }
        _sub_0 = tmp;
        val_0[31:0] = _sub_0;
        res_0 = val_0;
    }
    apply {
        res_0 = 64w0;
        update();
    }
}

control DP(packet_out b, in Headers p) {
    apply {
    }
}

control Verify(inout Headers hdrs, inout Metadata meta) {
    apply {
    }
}

control Compute(inout Headers hdr, inout Metadata meta) {
    apply {
    }
}

V1Switch<Headers, Metadata>(P(), Verify(), Ing(), Eg(), Compute(), DP()) main;

