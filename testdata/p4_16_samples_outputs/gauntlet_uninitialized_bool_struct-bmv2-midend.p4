#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
}

struct bool_struct {
    bool is_bool;
}

struct Headers {
    ethernet_t eth_hdr;
}

struct Meta {
}

parser p(packet_in pkt, out Headers hdr, inout Meta m, inout standard_metadata_t sm) {
    state start {
        pkt.extract<ethernet_t>(hdr.eth_hdr);
        transition accept;
    }
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    bool hasExited;
    @name("ingress.tmp") bool_struct tmp_0;
    @name("ingress.dummy_bit") bit<16> dummy_bit_0;
    @name("ingress.dummy_struct") bool_struct dummy_struct_0;
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("ingress.dummy_action") action dummy_action() {
        h.eth_hdr.eth_type = dummy_bit_0;
        tmp_0.is_bool = dummy_struct_0.is_bool;
    }
    @name("ingress.simple_table") table simple_table_0 {
        key = {
            h.eth_hdr.src_addr: exact @name("MsRuxx");
        }
        actions = {
            dummy_action();
            @defaultonly NoAction_1();
        }
        default_action = NoAction_1();
    }
    @hidden action gauntlet_uninitialized_bool_structbmv2l47() {
        hasExited = true;
    }
    @hidden action gauntlet_uninitialized_bool_structbmv2l49() {
        h.eth_hdr.dst_addr = 48w1;
    }
    @hidden action gauntlet_uninitialized_bool_structbmv2l31() {
        hasExited = false;
        tmp_0.is_bool = false;
    }
    @hidden table tbl_gauntlet_uninitialized_bool_structbmv2l31 {
        actions = {
            gauntlet_uninitialized_bool_structbmv2l31();
        }
        const default_action = gauntlet_uninitialized_bool_structbmv2l31();
    }
    @hidden table tbl_gauntlet_uninitialized_bool_structbmv2l47 {
        actions = {
            gauntlet_uninitialized_bool_structbmv2l47();
        }
        const default_action = gauntlet_uninitialized_bool_structbmv2l47();
    }
    @hidden table tbl_gauntlet_uninitialized_bool_structbmv2l49 {
        actions = {
            gauntlet_uninitialized_bool_structbmv2l49();
        }
        const default_action = gauntlet_uninitialized_bool_structbmv2l49();
    }
    apply {
        tbl_gauntlet_uninitialized_bool_structbmv2l31.apply();
        switch (simple_table_0.apply().action_run) {
            dummy_action: {
                if (tmp_0.is_bool) {
                    ;
                } else {
                    tbl_gauntlet_uninitialized_bool_structbmv2l47.apply();
                }
                if (hasExited) {
                    ;
                } else {
                    tbl_gauntlet_uninitialized_bool_structbmv2l49.apply();
                }
            }
            default: {
            }
        }
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
    }
}

V1Switch<Headers, Meta>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
