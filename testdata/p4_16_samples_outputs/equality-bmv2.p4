#include <core.p4>
#include <v1model.p4>

header H {
    bit<8>     s;
    varbit<32> v;
}

header Same {
    bit<8> same;
}

struct metadata {
}

struct headers {
    H    h;
    H[2] a;
    Same same;
}

parser p(packet_in b, out headers hdr, inout metadata meta, inout standard_metadata_t stdmeta) {
    state start {
        b.extract(hdr.h, 32);
        b.extract(hdr.a.next, 32);
        b.extract(hdr.a.next, 32);
        transition accept;
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t stdmeta) {
    apply {
        hdr.same.setValid();
        hdr.same.same = 0;
        stdmeta.egress_spec = 0;
        if (hdr.h.s == hdr.a[0].s) {
            hdr.same.same = hdr.same.same | 1;
        }
        if (hdr.h.v == hdr.a[0].v) {
            hdr.same.same = hdr.same.same | 2;
        }
        if (hdr.h == hdr.a[0]) {
            hdr.same.same = hdr.same.same | 4;
        }
        H[2] tmp;
        tmp[0] = hdr.h;
        tmp[1] = hdr.a[0];
        if (tmp == hdr.a) {
            hdr.same.same = hdr.same.same | 8;
        }
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t stdmeta) {
    apply {
    }
}

control vc(inout headers hdr, inout metadata meta) {
    apply {
    }
}

control uc(inout headers hdr, inout metadata meta) {
    apply {
    }
}

control deparser(packet_out packet, in headers hdr) {
    apply {
        packet.emit(hdr);
    }
}

V1Switch<headers, metadata>(p(), vc(), ingress(), egress(), uc(), deparser()) main;

