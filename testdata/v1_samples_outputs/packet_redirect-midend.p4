#include "/home/mbudiu/barefoot/git/p4c/build/../p4include/core.p4"
#include "/home/mbudiu/barefoot/git/p4c/build/../p4include/v1model.p4"

struct intrinsic_metadata_t {
    bit<4>  mcast_grp;
    bit<4>  egress_rid;
    bit<16> mcast_hash;
    bit<32> lf_field_list;
    bit<64> ingress_global_timestamp;
    bit<16> resubmit_flag;
    bit<16> recirculate_flag;
}

struct metaA_t {
    bit<8> f1;
    bit<8> f2;
}

struct metaB_t {
    bit<8> f1;
    bit<8> f2;
}

header hdrA_t {
    bit<8> f1;
    bit<8> f2;
}

struct metadata {
    @name("intrinsic_metadata") 
    intrinsic_metadata_t intrinsic_metadata;
    @name("metaA") 
    metaA_t              metaA;
    @name("metaB") 
    metaB_t              metaB;
}

struct headers {
    @name("hdrA") 
    hdrA_t hdrA;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("start") state start {
        packet.extract(hdr.hdrA);
        transition accept;
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    action NoAction_0() {
    }
    @name("_nop") action _nop_0() {
    }
    @name("_recirculate") action _recirculate_0() {
        recirculate({ standard_metadata, meta.metaA });
    }
    @name("_clone_e2e") action _clone_e2e_0(bit<8> mirror_id) {
        clone3(CloneType.E2E, (bit<32>)mirror_id, { standard_metadata, meta.metaA });
    }
    @name("t_egress") table t_egress_0() {
        actions = {
            _nop_0;
            _recirculate_0;
            _clone_e2e_0;
            NoAction_0;
        }
        key = {
            hdr.hdrA.f1                    : exact;
            standard_metadata.instance_type: ternary;
        }
        size = 128;
        default_action = NoAction_0();
    }
    apply {
        t_egress_0.apply();
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    action NoAction_1() {
    }
    @name("_nop") action _nop_1() {
    }
    @name("_set_port") action _set_port_0(bit<9> port) {
        standard_metadata.egress_spec = port;
        meta.metaA.f1 = 8w1;
    }
    @name("_multicast") action _multicast_0(bit<4> mgrp) {
        meta.intrinsic_metadata.mcast_grp = mgrp;
    }
    @name("_resubmit") action _resubmit_0() {
        resubmit({ standard_metadata, meta.metaA });
    }
    @name("_clone_i2e") action _clone_i2e_0(bit<8> mirror_id) {
        clone3(CloneType.I2E, (bit<32>)mirror_id, { standard_metadata, meta.metaA });
    }
    @name("t_ingress_1") table t_ingress() {
        actions = {
            _nop_1;
            _set_port_0;
            _multicast_0;
            NoAction_1;
        }
        key = {
            hdr.hdrA.f1  : exact;
            meta.metaA.f1: exact;
        }
        size = 128;
        default_action = NoAction_1();
    }
    @name("t_ingress_2") table t_ingress_0() {
        actions = {
            _nop_1;
            _resubmit_0;
            _clone_i2e_0;
            NoAction_1;
        }
        key = {
            hdr.hdrA.f1                    : exact;
            standard_metadata.instance_type: ternary;
        }
        size = 128;
        default_action = NoAction_1();
    }
    apply {
        t_ingress.apply();
        t_ingress_0.apply();
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit(hdr.hdrA);
    }
}

control verifyChecksum(in headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

V1Switch(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;
