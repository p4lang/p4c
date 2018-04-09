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
    bit<32> _sub;
    bit<64> res;
    bit<32> tmp_0;
    @name("Eg.update") action update_0(in bool p_1, inout bit<64> val) {
        _sub = val[31:0];
        if (p_1) 
            tmp_0 = _sub;
        else 
            tmp_0 = 32w1;
        _sub = tmp_0;
        val[31:0] = _sub;
    }
    apply {
        res = 64w0;
        update_0(true, res);
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

