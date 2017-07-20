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
    @name("standard_metadata") 
    standard_metadata_t  standard_metadata;
}

struct __headersImpl {
    @name("hdrA") 
    hdrA_t hdrA;
}

parser __ParserImpl(packet_in packet, out __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    @name(".start") state start {
        packet.extract<hdrA_t>(hdr.hdrA);
        transition accept;
    }
}

struct tuple_0 {
    standard_metadata_t field;
    metaA_t             field_0;
}

control ingress(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    @name("NoAction") action NoAction_0() {
    }
    @name("NoAction") action NoAction_3() {
    }
    @name("._nop") action _nop_0() {
    }
    @name("._nop") action _nop_2() {
    }
    @name("._set_port") action _set_port_0(bit<9> port) {
        meta.standard_metadata.egress_spec = port;
        meta.metaA.f1 = 8w1;
    }
    @name("._multicast") action _multicast_0(bit<4> mgrp) {
        meta.intrinsic_metadata.mcast_grp = mgrp;
    }
    @name("._resubmit") action _resubmit_0() {
        resubmit<tuple_0>({ meta.standard_metadata, meta.metaA });
    }
    @name("._clone_i2e") action _clone_i2e_0(bit<32> mirror_id) {
        clone3<tuple_0>(CloneType.I2E, mirror_id, { meta.standard_metadata, meta.metaA });
    }
    @name(".t_ingress_1") table t_ingress_1 {
        actions = {
            _nop_0();
            _set_port_0();
            _multicast_0();
            @defaultonly NoAction_0();
        }
        key = {
            hdr.hdrA.f1  : exact @name("hdr.hdrA.f1") ;
            meta.metaA.f1: exact @name("meta.metaA.f1") ;
        }
        size = 128;
        default_action = NoAction_0();
    }
    @name(".t_ingress_2") table t_ingress_2 {
        actions = {
            _nop_2();
            _resubmit_0();
            _clone_i2e_0();
            @defaultonly NoAction_3();
        }
        key = {
            hdr.hdrA.f1                         : exact @name("hdr.hdrA.f1") ;
            meta.standard_metadata.instance_type: ternary @name("meta.standard_metadata.instance_type") ;
        }
        size = 128;
        default_action = NoAction_3();
    }
    apply {
        t_ingress_1.apply();
        t_ingress_2.apply();
    }
}

control __egressImpl(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    apply {
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

V1Switch<__headersImpl, __metadataImpl>(__ParserImpl(), __verifyChecksumImpl(), ingress(), __egressImpl(), __computeChecksumImpl(), __DeparserImpl()) main;
