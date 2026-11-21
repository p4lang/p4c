#include <core.p4>
#include <v1model.p4>

header payload_t {
    bit<8> x;
    bit<8> y;
}
struct header_t {
    payload_t payload;
}
struct metadata {}

parser MyParser(packet_in packet,
                out header_t hdr,
                inout metadata meta,
                inout standard_metadata_t standard_metadata) {
    state start {
        packet.extract(hdr.payload);
        transition accept;
    }
}

control MyIngress(inout header_t hdr,
                  inout metadata meta,
                  inout standard_metadata_t standard_metadata) {

    action a1() {
        hdr.payload.x = 0xaa;
    }

    table t1 {
        key = { hdr.payload.x : exact; }
        actions = { a1; }
        size = 1024;
    }

    apply {
        if (hdr.payload.y == 0) {
            hdr.payload.x = hdr.payload.y;
            t1.apply();
        } else {
            t1.apply();
        }
        standard_metadata.egress_spec = 2;
    }
}

control MyVerifyChecksum(inout header_t hdr, inout metadata meta) { apply { } }
control MyEgress(inout header_t hdr, inout metadata meta,
                 inout standard_metadata_t standard_metadata) { apply {  } }
control MyDeparser(packet_out packet, in header_t hdr) {
    apply {
        packet.emit(hdr);
    }
}
control MyComputeChecksum(inout header_t hdr, inout metadata meta) { apply { } }

V1Switch(
MyParser(),
MyVerifyChecksum(),
MyIngress(),
MyEgress(),
MyComputeChecksum(),
MyDeparser()
) main;
