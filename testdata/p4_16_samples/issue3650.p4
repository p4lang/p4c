#include <core.p4>
#include <v1model.p4>

header payload_t {
    bit<8> x;
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

control C1(in bit<8> val) {
    apply {
        switch(val) {
            1 : {
            }
        }
    }
}

control MyIngress(inout header_t hdr,
                  inout metadata meta,
                  inout standard_metadata_t standard_metadata) {
    apply {
        switch(hdr.payload.x) {
            1 : {
                C1.apply(hdr.payload.x);
            }
            0 : {
                C1.apply(hdr.payload.x);
            }
        }
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
