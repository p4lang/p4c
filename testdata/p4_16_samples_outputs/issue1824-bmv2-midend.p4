error {
    IPv4OptionsNotSupported,
    IPv4ChecksumError,
    IPv4HeaderTooShort,
    IPv4BadPacket
}
#include <core.p4>
#include <v1model.p4>

header test_header {
    bit<48> dstAddr;
    bit<48> srcAddr;
}

struct headers {
    test_header h1;
}

struct mystruct1_t {
    bit<4> a;
    bit<4> b;
}

struct metadata {
    bit<4> _mystruct1_a0;
    bit<4> _mystruct1_b1;
}

parser MyParser(packet_in pkt, out headers hdr, inout metadata meta, inout standard_metadata_t stdmeta) {
    state start {
        pkt.extract<test_header>(hdr.h1);
        verify(false, error.IPv4BadPacket);
        verify(false, error.IPv4HeaderTooShort);
        transition accept;
    }
}

control MyIngress(inout headers hdr, inout metadata meta, inout standard_metadata_t stdmeta) {
    @hidden action act() {
        hdr.h1.dstAddr = 48w0xbad;
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    apply {
        if (stdmeta.parser_error != error.NoError) {
            tbl_act.apply();
        }
    }
}

control MyEgress(inout headers hdr, inout metadata meta, inout standard_metadata_t stdmeta) {
    apply {
    }
}

control MyVerifyChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

control MyComputeChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

control MyDeparser(packet_out packet, in headers hdr) {
    apply {
        packet.emit<test_header>(hdr.h1);
    }
}

V1Switch<headers, metadata>(MyParser(), MyVerifyChecksum(), MyIngress(), MyEgress(), MyComputeChecksum(), MyDeparser()) main;

