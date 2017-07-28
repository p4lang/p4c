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

struct __metadataImpl {
    @name("intrinsic_metadata") 
    intrinsic_metadata_t intrinsic_metadata;
    @name("metaA") 
    metaA_t              metaA;
    @name("metaB") 
    metaB_t              metaB;
}

struct __headersImpl {
    @name("hdrA") 
    hdrA_t hdrA;
}

parser __ParserImpl(packet_in packet, out __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t standard_metadata) {
    @name(".start") state start {
        packet.extract<hdrA_t>(hdr.hdrA);
        transition accept;
    }
}

control egress(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t standard_metadata) {
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
            hdr.hdrA.f1                    : exact @name("hdr.hdrA.f1") ;
            standard_metadata.instance_type: ternary @name("standard_metadata.instance_type") ;
        }
        size = 128;
        default_action = NoAction();
    }
    apply {
        t_egress.apply();
    }
}

control ingress(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t standard_metadata) {
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
            hdr.hdrA.f1  : exact @name("hdr.hdrA.f1") ;
            meta.metaA.f1: exact @name("meta.metaA.f1") ;
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
            hdr.hdrA.f1                    : exact @name("hdr.hdrA.f1") ;
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

control __DeparserImpl(packet_out packet, in __headersImpl hdr) {
    apply {
        packet.emit<hdrA_t>(hdr.hdrA);
    }
}

control __verifyChecksumImpl(in __headersImpl hdr, inout __metadataImpl meta) {
    apply {
    }
}

control __computeChecksumImpl(inout __headersImpl hdr, inout __metadataImpl meta) {
    apply {
    }
}

V1Switch<__headersImpl, __metadataImpl>(__ParserImpl(), __verifyChecksumImpl(), ingress(), egress(), __computeChecksumImpl(), __DeparserImpl()) main;
