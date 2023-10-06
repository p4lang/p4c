#include <core.p4>
#include <v1model.p4>

header hdr {
    bit<8>  a;
}

struct Header_t {
    hdr h;
}

header SimpleHeader {
    bit<8> b;
}

struct ArrayStruct {
    tuple<SimpleHeader[10]> n;
}

struct Meta_t {
    ArrayStruct s;
}

parser p(packet_in b, out Header_t h, inout Meta_t m, inout standard_metadata_t sm) {
    state start {
        b.extract(h.h);
        transition accept;
    }
}

control vrfy(inout Header_t h, inout Meta_t m) { apply {} }
control update(inout Header_t h, inout Meta_t m) { apply {} }
control egress(inout Header_t h, inout Meta_t m, inout standard_metadata_t sm) { apply {} }
control deparser(packet_out b, in Header_t h) { apply { b.emit(h.h); } }

control ingress(inout Header_t h, inout Meta_t m, inout standard_metadata_t standard_meta) {
    apply {
        // This access causes a segmentation fault.
        m.s.n[0][0].b = 1;
    }
}

V1Switch(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
