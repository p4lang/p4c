#include <core.p4>
#include <v1model.p4>

struct intrinsic_metadata_t {
    bit<16> mcast_grp;
    bit<4>  egress_rid;
    bit<32> lf_field_list;
    bit<64> ingress_global_timestamp;
    bit<16> resubmit_flag;
    bit<16> recirculate_flag;
}

struct metaA_t {
    @field_list(8w0) 
    bit<8> f1;
    @field_list(8w0) 
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
<<<<<<< e2f4d7dd38c28b1a9d07067e5136a748a68e76b1
    bit<8> _metaA_f10;
    bit<8> _metaA_f21;
    bit<8> _metaB_f12;
    bit<8> _metaB_f23;
=======
    bit<4>  _intrinsic_metadata_mcast_grp0;
    bit<4>  _intrinsic_metadata_egress_rid1;
    bit<32> _intrinsic_metadata_lf_field_list2;
    bit<64> _intrinsic_metadata_ingress_global_timestamp3;
    bit<16> _intrinsic_metadata_resubmit_flag4;
    bit<16> _intrinsic_metadata_recirculate_flag5;
    @field_list(8w0) 
    bit<8>  _metaA_f16;
    @field_list(8w0) 
    bit<8>  _metaA_f27;
    bit<8>  _metaB_f18;
    bit<8>  _metaB_f29;
>>>>>>> Handle multiple resubmits/recirculate/clone calls
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
    @name(".NoAction") action NoAction_0() {
    }
    @name("._nop") action _nop() {
    }
    @name("._recirculate") action _recirculate() {
<<<<<<< e2f4d7dd38c28b1a9d07067e5136a748a68e76b1
<<<<<<< 43bd696b29be944551572728dfc9ec48437ee961
        recirculate<tuple_0>({ standard_metadata, {meta._metaA_f10,meta._metaA_f21} });
    }
    @name("._clone_e2e") action _clone_e2e(bit<32> mirror_id) {
        clone3<tuple_0>(CloneType.E2E, mirror_id, { standard_metadata, {meta._metaA_f10,meta._metaA_f21} });
=======
<<<<<<< 1968b35515ddd4809e438d338481981969628fc8
        recirculate<tuple_0>({ standard_metadata, {meta._metaA_f16,meta._metaA_f27} });
    }
    @name("._clone_e2e") action _clone_e2e(bit<32> mirror_id) {
        clone3<tuple_0>(CloneType.E2E, mirror_id, { standard_metadata, {meta._metaA_f16,meta._metaA_f27} });
=======
        recirculate();
    }
    @name("._clone_e2e") action _clone_e2e(bit<32> mirror_id) {
        clone3(CloneType.E2E, mirror_id);
>>>>>>> Tag metadata fields that need to be recirculated
>>>>>>> Tag metadata fields that need to be recirculated
=======
        recirculate(8w0);
    }
    @name("._clone_e2e") action _clone_e2e(bit<32> mirror_id) {
        clone3(CloneType.E2E, mirror_id, 8w0);
>>>>>>> Handle multiple resubmits/recirculate/clone calls
    }
    @name(".t_egress") table t_egress_0 {
        actions = {
            _nop();
            _recirculate();
            _clone_e2e();
            @defaultonly NoAction_0();
        }
        key = {
            hdr.hdrA.f1                    : exact @name("hdrA.f1") ;
            standard_metadata.instance_type: ternary @name("standard_metadata.instance_type") ;
        }
        size = 128;
        default_action = NoAction_0();
    }
    apply {
        t_egress_0.apply();
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".NoAction") action NoAction_1() {
    }
    @name(".NoAction") action NoAction_5() {
    }
    @name("._nop") action _nop_2() {
    }
    @name("._nop") action _nop_4() {
    }
    @name("._set_port") action _set_port(bit<9> port) {
        standard_metadata.egress_spec = port;
        meta._metaA_f10 = 8w1;
    }
    @name("._multicast") action _multicast(bit<16> mgrp) {
        standard_metadata.mcast_grp = mgrp;
    }
    @name("._resubmit") action _resubmit() {
<<<<<<< e2f4d7dd38c28b1a9d07067e5136a748a68e76b1
<<<<<<< 43bd696b29be944551572728dfc9ec48437ee961
        resubmit<tuple_0>({ standard_metadata, {meta._metaA_f10,meta._metaA_f21} });
    }
    @name("._clone_i2e") action _clone_i2e(bit<32> mirror_id) {
        clone3<tuple_0>(CloneType.I2E, mirror_id, { standard_metadata, {meta._metaA_f10,meta._metaA_f21} });
=======
<<<<<<< 1968b35515ddd4809e438d338481981969628fc8
        resubmit<tuple_0>({ standard_metadata, {meta._metaA_f16,meta._metaA_f27} });
    }
    @name("._clone_i2e") action _clone_i2e(bit<32> mirror_id) {
        clone3<tuple_0>(CloneType.I2E, mirror_id, { standard_metadata, {meta._metaA_f16,meta._metaA_f27} });
=======
        resubmit();
    }
    @name("._clone_i2e") action _clone_i2e(bit<32> mirror_id) {
        clone3(CloneType.I2E, mirror_id);
>>>>>>> Tag metadata fields that need to be recirculated
>>>>>>> Tag metadata fields that need to be recirculated
=======
        resubmit(8w0);
    }
    @name("._clone_i2e") action _clone_i2e(bit<32> mirror_id) {
        clone3(CloneType.I2E, mirror_id, 8w0);
>>>>>>> Handle multiple resubmits/recirculate/clone calls
    }
    @name(".t_ingress_1") table t_ingress {
        actions = {
            _nop_2();
            _set_port();
            _multicast();
            @defaultonly NoAction_1();
        }
        key = {
            hdr.hdrA.f1    : exact @name("hdrA.f1") ;
            meta._metaA_f10: exact @name("metaA.f1") ;
        }
        size = 128;
        default_action = NoAction_1();
    }
    @name(".t_ingress_2") table t_ingress_0 {
        actions = {
            _nop_4();
            _resubmit();
            _clone_i2e();
            @defaultonly NoAction_5();
        }
        key = {
            hdr.hdrA.f1                    : exact @name("hdrA.f1") ;
            standard_metadata.instance_type: ternary @name("standard_metadata.instance_type") ;
        }
        size = 128;
        default_action = NoAction_5();
    }
    apply {
        t_ingress.apply();
        t_ingress_0.apply();
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

