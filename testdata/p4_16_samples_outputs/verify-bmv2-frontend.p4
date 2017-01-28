error {
    NewError
}
#include <core.p4>
#include <v1model.p4>

struct m {
    int<8> x;
}

struct h {
}

parser MyParser(packet_in b, out h hdr, inout m meta, inout standard_metadata_t std) {
    error e_0;
    bool tmp;
    state start {
        tmp = meta.x == 8s0;
        verify(tmp, error.NewError);
        verify(true, error.NoError);
        e_0 = error.NoError;
        verify(true, e_0);
        transition accept;
    }
}

control MyVerifyChecksum(in h hdr, inout m meta) {
    apply {
    }
}

control MyIngress(inout h hdr, inout m meta, inout standard_metadata_t std) {
    apply {
    }
}

control MyEgress(inout h hdr, inout m meta, inout standard_metadata_t std) {
    apply {
    }
}

control MyComputeChecksum(inout h hdr, inout m meta) {
    apply {
    }
}

control MyDeparser(packet_out b, in h hdr) {
    apply {
    }
}

V1Switch<h, m>(MyParser(), MyVerifyChecksum(), MyIngress(), MyEgress(), MyComputeChecksum(), MyDeparser()) main;
