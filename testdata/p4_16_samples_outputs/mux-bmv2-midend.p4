#include <core.p4>
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
    bit<64> res;
    bit<32> tmp_0;
    bit<64> val;
    @name("Eg.update") action update_0() {
        val = res;
        tmp_0 = res[31:0];
        tmp_0 = tmp_0;
        val[31:0] = tmp_0;
        res = val;
    }
    @hidden action act() {
        res = 64w0;
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    @hidden table tbl_update {
        actions = {
            update_0();
        }
        const default_action = update_0();
    }
    apply {
        tbl_act.apply();
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

