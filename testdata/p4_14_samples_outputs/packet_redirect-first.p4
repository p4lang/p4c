#include <core.p4>
#include <v1model.p4>

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
    @name(".intrinsic_metadata") 
    intrinsic_metadata_t intrinsic_metadata;
    @name(".metaA") 
    metaA_t              metaA;
    @name(".metaB") 
    metaB_t              metaB;
}

struct headers {
    @name(".hdrA") 
    hdrA_t hdrA;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".start") state start {
        packet.extract<hdrA_t>(hdr.hdrA);
        transition accept;
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("._nop") action _nop() {
    }
    @name("._recirculate") action _recirculate() {
        recirculate<tuple<standard_metadata_t, metaA_t>>({ standard_metadata, meta.metaA });
    }
    @name("._clone_e2e") action _clone_e2e(bit<32> mirror_id) {
        clone3<tuple<standard_metadata_t, metaA_t>>(CloneType.E2E, mirror_id, { standard_metadata, meta.metaA });
    }
    @name(".t_egress") table t_egress {
        actions = {
            _nop();
            _recirculate();
            _clone_e2e();
            @defaultonly NoAction();
        }
        key = {
            hdr.hdrA.f1                    : exact @name("hdrA.f1") ;
            standard_metadata.instance_type: ternary @name("standard_metadata.instance_type") ;
        }
        size = 128;
        default_action = NoAction();
    }
    apply {
        t_egress.apply();
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("._nop") action _nop() {
    }
    @name("._set_port") action _set_port(bit<9> port) {
        standard_metadata.egress_spec = port;
        meta.metaA.f1 = 8w1;
    }
    @name("._multicast") action _multicast(bit<4> mgrp) {
        meta.intrinsic_metadata.mcast_grp = mgrp;
    }
    @name("._resubmit") action _resubmit() {
        resubmit<tuple<standard_metadata_t, metaA_t>>({ standard_metadata, meta.metaA });
    }
    @name("._clone_i2e") action _clone_i2e(bit<32> mirror_id) {
        clone3<tuple<standard_metadata_t, metaA_t>>(CloneType.I2E, mirror_id, { standard_metadata, meta.metaA });
    }
    @name(".t_ingress_1") table t_ingress_1 {
        actions = {
            _nop();
            _set_port();
            _multicast();
            @defaultonly NoAction();
        }
        key = {
            hdr.hdrA.f1  : exact @name("hdrA.f1") ;
            meta.metaA.f1: exact @name("metaA.f1") ;
        }
        size = 128;
        default_action = NoAction();
    }
    @name(".t_ingress_2") table t_ingress_2 {
        actions = {
            _nop();
            _resubmit();
            _clone_i2e();
            @defaultonly NoAction();
        }
        key = {
            hdr.hdrA.f1                    : exact @name("hdrA.f1") ;
            standard_metadata.instance_type: ternary @name("standard_metadata.instance_type") ;
        }
        size = 128;
        default_action = NoAction();
    }
    apply {
        t_ingress_1.apply();
        t_ingress_2.apply();
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit<hdrA_t>(hdr.hdrA);
    }
}

control verifyChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

V1Switch<headers, metadata>(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;

