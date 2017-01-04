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
    bit<32> _sub_0;
    bit<64> res_0;
    bit<32> tmp;
    bit<64> tmp_0;
    @name("update") action update_0(in bool p, inout bit<64> val) {
        _sub_0 = val[31:0];
        if (p) 
            tmp = _sub_0;
        else 
            tmp = 32w1;
        _sub_0 = tmp;
        val[31:0] = _sub_0;
    }
    apply {
        res_0 = 64w0;
        tmp_0 = res_0;
        update_0(true, tmp_0);
    }
}

control DP(packet_out b, in Headers p) {
    apply {
    }
}

control Verify(in Headers hdrs, inout Metadata meta) {
    apply {
    }
}

control Compute(inout Headers hdr, inout Metadata meta) {
    apply {
    }
}

V1Switch<Headers, Metadata>(P(), Verify(), Ing(), Eg(), Compute(), DP()) main;
