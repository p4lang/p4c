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
        b.extract<H>(hdr.h, 32w32);
        b.extract<H>(hdr.a.next, 32w32);
        b.extract<H>(hdr.a.next, 32w32);
        transition accept;
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t stdmeta) {
    H[2] tmp_0;
    @hidden action act() {
        hdr.same.same = 8w1;
    }
    @hidden action act_0() {
        hdr.same.setValid();
        hdr.same.same = 8w0;
        stdmeta.egress_spec = 9w0;
    }
    @hidden action act_1() {
        hdr.same.same = hdr.same.same | 8w2;
    }
    @hidden action act_2() {
        hdr.same.same = hdr.same.same | 8w4;
    }
    @hidden action act_3() {
        hdr.same.same = hdr.same.same | 8w8;
    }
    @hidden action act_4() {
        tmp_0[0] = hdr.h;
        tmp_0[1] = hdr.a[0];
    }
    @hidden table tbl_act {
        actions = {
            act_0();
        }
        const default_action = act_0();
    }
    @hidden table tbl_act_0 {
        actions = {
            act();
        }
        const default_action = act();
    }
    @hidden table tbl_act_1 {
        actions = {
            act_1();
        }
        const default_action = act_1();
    }
    @hidden table tbl_act_2 {
        actions = {
            act_2();
        }
        const default_action = act_2();
    }
    @hidden table tbl_act_3 {
        actions = {
            act_4();
        }
        const default_action = act_4();
    }
    @hidden table tbl_act_4 {
        actions = {
            act_3();
        }
        const default_action = act_3();
    }
    apply {
        tbl_act.apply();
        if (hdr.h.s == hdr.a[0].s) {
            tbl_act_0.apply();
        }
        if (hdr.h.v == hdr.a[0].v) {
            tbl_act_1.apply();
        }
        if (!hdr.h.isValid() && !hdr.a[0].isValid() || hdr.h.isValid() && hdr.a[0].isValid() && hdr.h.s == hdr.a[0].s && hdr.h.v == hdr.a[0].v) {
            tbl_act_2.apply();
        }
        tbl_act_3.apply();
        if ((!tmp_0[0].isValid() && !hdr.a[0].isValid() || tmp_0[0].isValid() && hdr.a[0].isValid() && tmp_0[0].s == hdr.a[0].s && tmp_0[0].v == hdr.a[0].v) && (!tmp_0[1].isValid() && !hdr.a[1].isValid() || tmp_0[1].isValid() && hdr.a[1].isValid() && tmp_0[1].s == hdr.a[1].s && tmp_0[1].v == hdr.a[1].v)) {
            tbl_act_4.apply();
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
        packet.emit<H>(hdr.h);
        packet.emit<H>(hdr.a[0]);
        packet.emit<H>(hdr.a[1]);
        packet.emit<Same>(hdr.same);
    }
}

V1Switch<headers, metadata>(p(), vc(), ingress(), egress(), uc(), deparser()) main;

