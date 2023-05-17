#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
}

header H {
    bit<8> a;
    bit<8> b;
}

struct Headers {
    ethernet_t eth_hdr;
    H          h;
}

struct Meta {
}

parser p(packet_in pkt, out Headers hdr, inout Meta m, inout standard_metadata_t sm) {
    state start {
        pkt.extract<ethernet_t>(hdr.eth_hdr);
        pkt.extract<H>(hdr.h);
        transition accept;
    }
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    @hidden action gauntlet_hdr_assign_2bmv2l41() {
        h.h.setValid();
    }
    @hidden action gauntlet_hdr_assign_2bmv2l43() {
        h.h.setValid();
    }
    @hidden action gauntlet_hdr_assign_2bmv2l37() {
        h.h.a = 8w1;
        h.h.setInvalid();
    }
    @hidden action gauntlet_hdr_assign_2bmv2l45() {
        h.h.b = 8w2;
    }
    @hidden table tbl_gauntlet_hdr_assign_2bmv2l37 {
        actions = {
            gauntlet_hdr_assign_2bmv2l37();
        }
        const default_action = gauntlet_hdr_assign_2bmv2l37();
    }
    @hidden table tbl_gauntlet_hdr_assign_2bmv2l41 {
        actions = {
            gauntlet_hdr_assign_2bmv2l41();
        }
        const default_action = gauntlet_hdr_assign_2bmv2l41();
    }
    @hidden table tbl_gauntlet_hdr_assign_2bmv2l43 {
        actions = {
            gauntlet_hdr_assign_2bmv2l43();
        }
        const default_action = gauntlet_hdr_assign_2bmv2l43();
    }
    @hidden table tbl_gauntlet_hdr_assign_2bmv2l45 {
        actions = {
            gauntlet_hdr_assign_2bmv2l45();
        }
        const default_action = gauntlet_hdr_assign_2bmv2l45();
    }
    apply {
        tbl_gauntlet_hdr_assign_2bmv2l37.apply();
        if (sm.egress_port == 9w2) {
            tbl_gauntlet_hdr_assign_2bmv2l41.apply();
        } else {
            tbl_gauntlet_hdr_assign_2bmv2l43.apply();
        }
        tbl_gauntlet_hdr_assign_2bmv2l45.apply();
    }
}

control vrfy(inout Headers h, inout Meta m) {
    apply {
    }
}

control update(inout Headers h, inout Meta m) {
    apply {
    }
}

control egress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    apply {
    }
}

control deparser(packet_out pkt, in Headers h) {
    apply {
        pkt.emit<ethernet_t>(h.eth_hdr);
        pkt.emit<H>(h.h);
    }
}

V1Switch<Headers, Meta>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
