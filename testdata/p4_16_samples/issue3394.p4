#include <core.p4>
#include <v1model.p4>

header header_t {}
struct metadata {}

parser MyParser(packet_in packet,
                out header_t hdr,
                inout metadata meta,
                inout standard_metadata_t standard_metadata) {
    state start {
        transition accept;
    }
}

control C1(int a, inout bit<8> b) {
    apply {
        if (b == a) {
            b = a+8w1;
        } else {
            b = 1+a;
        }
    }
}

control MyIngress(inout header_t hdr,
                  inout metadata meta,
                  inout standard_metadata_t standard_metadata) {
    apply {
        bit<8> r = 1;
        C1.apply(3, r);
    }
}

control MyVerifyChecksum(inout header_t hdr, inout metadata meta) { apply { } }
control MyEgress(inout header_t hdr, inout metadata meta,
                 inout standard_metadata_t standard_metadata) { apply {  } }
control MyDeparser(packet_out packet, in header_t hdr) { apply { } }
control MyComputeChecksum(inout header_t hdr, inout metadata meta) { apply { } }

V1Switch(
MyParser(),
MyVerifyChecksum(),
MyIngress(),
MyEgress(),
MyComputeChecksum(),
MyDeparser()
) main;
