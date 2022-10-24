#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
}

struct Headers {
    ethernet_t eth_hdr1;
    ethernet_t eth_hdr2;
}

struct Meta {
}

parser p(packet_in pkt, out Headers hdr, inout Meta m, inout standard_metadata_t sm) {
    state start {
        pkt.extract<ethernet_t>(hdr.eth_hdr1);
        pkt.extract<ethernet_t>(hdr.eth_hdr2);
        transition accept;
    }
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    @name("ingress.hasReturned") bool hasReturned;
    @name("ingress.retval") ethernet_t retval;
    @name("ingress.retval_0") ethernet_t retval_0;
    @hidden action gauntlet_hdr_function_castbmv2l22() {
        hasReturned = true;
        retval.setValid();
        retval.dst_addr = 48w1;
        retval.src_addr = 48w1;
        retval.eth_type = 16w1;
    }
    @hidden action gauntlet_hdr_function_castbmv2l24() {
        hasReturned = true;
        retval.setValid();
        retval.dst_addr = 48w2;
        retval.src_addr = 48w2;
        retval.eth_type = 16w2;
    }
    @hidden action act() {
        hasReturned = false;
    }
    @hidden action gauntlet_hdr_function_castbmv2l26() {
        retval.setValid();
        retval.dst_addr = 48w3;
        retval.src_addr = 48w3;
        retval.eth_type = 16w3;
    }
    @hidden action gauntlet_hdr_function_castbmv2l30() {
        h.eth_hdr1 = retval;
        retval_0.setValid();
        retval_0.dst_addr = 48w1;
        retval_0.src_addr = 48w1;
        retval_0.eth_type = 16w1;
        h.eth_hdr2 = retval_0;
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    @hidden table tbl_gauntlet_hdr_function_castbmv2l22 {
        actions = {
            gauntlet_hdr_function_castbmv2l22();
        }
        const default_action = gauntlet_hdr_function_castbmv2l22();
    }
    @hidden table tbl_gauntlet_hdr_function_castbmv2l24 {
        actions = {
            gauntlet_hdr_function_castbmv2l24();
        }
        const default_action = gauntlet_hdr_function_castbmv2l24();
    }
    @hidden table tbl_gauntlet_hdr_function_castbmv2l26 {
        actions = {
            gauntlet_hdr_function_castbmv2l26();
        }
        const default_action = gauntlet_hdr_function_castbmv2l26();
    }
    @hidden table tbl_gauntlet_hdr_function_castbmv2l30 {
        actions = {
            gauntlet_hdr_function_castbmv2l30();
        }
        const default_action = gauntlet_hdr_function_castbmv2l30();
    }
    apply {
        tbl_act.apply();
        if (h.eth_hdr1.eth_type == 16w1) {
            tbl_gauntlet_hdr_function_castbmv2l22.apply();
        } else if (h.eth_hdr1.eth_type == 16w2) {
            tbl_gauntlet_hdr_function_castbmv2l24.apply();
        }
        if (hasReturned) {
            ;
        } else {
            tbl_gauntlet_hdr_function_castbmv2l26.apply();
        }
        tbl_gauntlet_hdr_function_castbmv2l30.apply();
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
        pkt.emit<ethernet_t>(h.eth_hdr1);
        pkt.emit<ethernet_t>(h.eth_hdr2);
    }
}

V1Switch<Headers, Meta>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
