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
    @name("Eg.res") bit<64> res_0;
    @name("Eg.val") bit<64> val_0;
    @name("Eg.update") action update() {
        val_0 = res_0;
        val_0[31:0] = res_0[31:0];
        res_0 = val_0;
    }
    @hidden action muxbmv2l58() {
        res_0 = 64w0;
    }
    @hidden table tbl_muxbmv2l58 {
        actions = {
            muxbmv2l58();
        }
        const default_action = muxbmv2l58();
    }
    @hidden table tbl_update {
        actions = {
            update();
        }
        const default_action = update();
    }
    apply {
        tbl_muxbmv2l58.apply();
        tbl_update.apply();
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

