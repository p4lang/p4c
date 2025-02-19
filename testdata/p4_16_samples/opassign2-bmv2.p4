#include <core.p4>
#include <v1model.p4>

header data_t {
    bit<32>     f1;
    bit<32>     f2;
    bit<16>     h1;
    bit<16>     h2;
    bit<8>      b1;
    bit<8>      b2;
    bit<8>      b3;
    bit<8>      b4;
}

header word_t {
    bit<32>     x;
}

struct metadata {}

struct headers {
    data_t      data;
    word_t[8]   rest;
}

parser p(packet_in b,
         out headers hdr,
         inout metadata meta,
         inout standard_metadata_t stdmeta) {
    state start {
        b.extract(hdr.data);
        b.extract(hdr.rest.next);
        b.extract(hdr.rest.next);
        b.extract(hdr.rest.next);
        b.extract(hdr.rest.next);
        b.extract(hdr.rest.next);
        b.extract(hdr.rest.next);
        b.extract(hdr.rest.next);
        b.extract(hdr.rest.next);
        transition accept;
    }
}

bit<8> postincr(inout bit<8> x) {
    bit<8> rv = x;
    x += 1;
    return rv;
}

control ingress(inout headers hdr,
                 inout metadata meta,
                 inout standard_metadata_t stdmeta) {
    apply {
        if (hdr.rest[7].isValid()) {
            hdr.rest[postincr(hdr.data.b1) & 7].x |= hdr.data.f1;
            hdr.rest[postincr(hdr.data.b1) & 7].x |= hdr.data.f2;
        }
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
