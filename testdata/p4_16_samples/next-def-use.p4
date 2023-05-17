/**
 * Basic test that assigns a value to <hdr_stack>.next
 */
#include <v1model.p4>

struct metadata { }

header data_t {
    bit<8> f;
}

struct headers {
    data_t[3]   hs;
}

parser ParserImpl(      packet_in           packet,
                  out   headers             hdr,
                  inout metadata            meta,
                  inout standard_metadata_t standard_metadata) {
    state start {
        packet.extract(hdr.hs[0]);
        transition p0;
    }

    state p0 {
        hdr.hs.next = {8w0};
        transition accept;
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
        if (hdr.hs[1].isValid()) {
            hdr.hs[2].f = 10;
        } else if (hdr.hs[0].isValid()) {
            standard_metadata.egress_spec = 3;
        }
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit(hdr);
    }
}

control verifyChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

V1Switch(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;