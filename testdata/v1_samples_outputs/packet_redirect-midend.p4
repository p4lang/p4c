#include "/home/cdodd/p4c/build/../p4include/core.p4"
#include "/home/cdodd/p4c/build/../p4include/v1model.p4"

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
    @name("_nop") action _nop() {
        bool hasReturned_0 = false;
    }
    @name("_recirculate") action _recirculate() {
        bool hasReturned_1 = false;
        recirculate({ standard_metadata, meta.metaA });
    }
    @name("_clone_e2e") action _clone_e2e(bit<8> mirror_id) {
        bool hasReturned_2 = false;
        clone3(CloneType.E2E, (bit<32>)mirror_id, { standard_metadata, meta.metaA });
    }
    @name("t_egress") table t_egress() {
        actions = {
            _nop;
            _recirculate;
            _clone_e2e;
            NoAction;
        }
        key = {
            hdr.hdrA.f1                    : exact;
            standard_metadata.instance_type: ternary;
        }
        size = 128;
        default_action = NoAction();
    }

    apply {
        bool hasExited = false;
        t_egress.apply();
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("_nop") action _nop() {
        bool hasReturned_3 = false;
    }
    @name("_set_port") action _set_port(bit<9> port) {
        bool hasReturned_4 = false;
        standard_metadata.egress_spec = port;
        meta.metaA.f1 = 8w1;
    }
    @name("_multicast") action _multicast(bit<4> mgrp) {
        bool hasReturned_5 = false;
        meta.intrinsic_metadata.mcast_grp = mgrp;
    }
    @name("_resubmit") action _resubmit() {
        bool hasReturned_6 = false;
        resubmit({ standard_metadata, meta.metaA });
    }
    @name("_clone_i2e") action _clone_i2e(bit<8> mirror_id) {
        bool hasReturned_7 = false;
        clone3(CloneType.I2E, (bit<32>)mirror_id, { standard_metadata, meta.metaA });
    }
    @name("t_ingress_1") table t_ingress_1() {
        actions = {
            _nop;
            _set_port;
            _multicast;
            NoAction;
        }
        key = {
            hdr.hdrA.f1  : exact;
            meta.metaA.f1: exact;
        }
        size = 128;
        default_action = NoAction();
    }

    @name("t_ingress_2") table t_ingress_2() {
        actions = {
            _nop;
            _resubmit;
            _clone_i2e;
            NoAction;
        }
        key = {
            hdr.hdrA.f1                    : exact;
            standard_metadata.instance_type: ternary;
        }
        size = 128;
        default_action = NoAction();
    }

    apply {
        bool hasExited_0 = false;
        t_ingress_1.apply();
        t_ingress_2.apply();
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        bool hasExited_1 = false;
        packet.emit(hdr.hdrA);
    }
}

control verifyChecksum(in headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
        bool hasExited_2 = false;
    }
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    apply {
        bool hasExited_3 = false;
    }
}

V1Switch(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;
