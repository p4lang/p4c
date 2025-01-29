#include <core.p4>
#include <v1model.p4>

header data_t {
    bit<32>     f1;
    bit<32>     f2;
    bit<16>     h1;
    bit<16>     h2;
    bit<8>      arr[16];
}

struct metadata {}

struct headers {
    data_t      data;
}

parser p(packet_in b,
         out headers hdr,
         inout metadata meta,
         inout standard_metadata_t stdmeta) {
    state start {
        b.extract(hdr.data);
        transition accept;
    }
}

control ingress(inout headers hdr,
                 inout metadata meta,
                 inout standard_metadata_t stdmeta) {
    apply {
        hdr.data.arr[0] = hdr.data.f1[0+:8];
        hdr.data.arr[1] = hdr.data.f1[8+:8];
        hdr.data.arr[2] = hdr.data.f1[16+:8];
        hdr.data.arr[3] = hdr.data.f1[24+:8];
        hdr.data.arr[4] = hdr.data.f2[0+:8];
        hdr.data.arr[5] = hdr.data.f2[8+:8];
        hdr.data.arr[6] = hdr.data.f2[16+:8];
        hdr.data.arr[7] = hdr.data.f2[24+:8];
    }
}

control egress(inout headers hdr,
               inout metadata meta,
               inout standard_metadata_t stdmeta) {
    apply {}
}

control vc(inout headers hdr,
           inout metadata meta) {
    apply {}
}

control uc(inout headers hdr,
           inout metadata meta) {
    apply {}
}

control deparser(packet_out packet,
                 in headers hdr) {
    apply {
        packet.emit(hdr);
    }
}

V1Switch<headers, metadata>(p(),
                            vc(),
                            ingress(),
                            egress(),
                            uc(),
                            deparser()) main;
