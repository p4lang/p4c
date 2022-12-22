#include <v1model.p4>

typedef bit<12> vlan_id_t;

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
}

header H {
    bit<8> a;
    bit<8> b;
}

header vlan_tag_h {
    bit<3> pcp;
    bit<1> cfi;
    vlan_id_t vid;
    bit<16> ether_type;
}

struct Headers {
    ethernet_t eth_hdr;
    H h;
    vlan_tag_h[2] vlan_tag;
}

struct Meta {}

parser p(packet_in pkt, out Headers h, inout Meta meta, inout standard_metadata_t stdmeta) {
    state start {
        pkt.extract(h.eth_hdr);
        transition parse;
    }
    state parse {
        pkt.extract(h.h);
        pkt.extract(h.vlan_tag[0]);
        pkt.extract(h.vlan_tag[1]);
        transition accept;
    }
}

control vrfy(inout Headers h, inout Meta meta) {
    apply { }
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t s) {

    action MyAction1() {
        h.h.b = 1;
    }

    table vlan_stg {
        key = {
            h.vlan_tag[0].vid : exact;
        }

        actions = {
            MyAction1;
        }

        const default_action = MyAction1();
    }

    apply {
        vlan_stg.apply();
    }
}

control egress(inout Headers h, inout Meta m, inout standard_metadata_t s) {
    apply { }
}

control update(inout Headers h, inout Meta m) {
    apply { }
}

control deparser(packet_out pkt, in Headers h) {
    apply {
        pkt.emit(h);
    }
}

V1Switch(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
