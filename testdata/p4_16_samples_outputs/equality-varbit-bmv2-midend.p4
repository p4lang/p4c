#include <core.p4>
#include <v1model.p4>

header H {
    bit<8>     s;
    varbit<32> v;
}

struct metadata {
}

struct headers {
    H h;
}

parser p(packet_in b, out headers hdr, inout metadata meta, inout standard_metadata_t stdmeta) {
    state start {
        b.extract<H>(hdr.h, 32w32);
        transition accept;
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t stdmeta) {
    H h_0;
    @hidden action act() {
        stdmeta.egress_spec = 9w1;
    }
    @hidden action act_0() {
        stdmeta.egress_spec = 9w0;
        h_0 = hdr.h;
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
    apply {
        tbl_act.apply();
        if (hdr.h.v == h_0.v) 
            tbl_act_0.apply();
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
    }
}

V1Switch<headers, metadata>(p(), vc(), ingress(), egress(), uc(), deparser()) main;

