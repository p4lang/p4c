#include <core.p4>
#include <v1model.p4>

struct intrinsic_metadata_t {
    bit<16> mcast_grp;
    bit<4>  egress_rid;
    bit<64> ingress_global_timestamp;
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
    @name(".metaA") 
    metaA_t metaA;
    @name(".metaB") 
    metaB_t metaB;
}

struct headers {
    @name(".hdrA") 
    hdrA_t hdrA;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".start") state start {
        packet.extract(hdr.hdrA);
        transition accept;
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("._nop") action _nop() {
    }
    @name("._recirculate") action _recirculate() {
        recirculate({ standard_metadata, meta.metaA });
    }
    @name("._clone_e2e") action _clone_e2e(bit<32> mirror_id) {
        clone3(CloneType.E2E, (bit<32>)mirror_id, { standard_metadata, meta.metaA });
    }
    @name(".t_egress") table t_egress {
        actions = {
            _nop;
            _recirculate;
            _clone_e2e;
        }
        key = {
            hdr.hdrA.f1                    : exact;
            standard_metadata.instance_type: ternary;
        }
        size = 128;
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
    @name("._multicast") action _multicast(bit<16> mgrp) {
        standard_metadata.mcast_grp = mgrp;
    }
    @name("._resubmit") action _resubmit() {
        resubmit({ standard_metadata, meta.metaA });
    }
    @name("._clone_i2e") action _clone_i2e(bit<32> mirror_id) {
        clone3(CloneType.I2E, (bit<32>)mirror_id, { standard_metadata, meta.metaA });
    }
    @name(".t_ingress_1") table t_ingress_1 {
        actions = {
            _nop;
            _set_port;
            _multicast;
        }
        key = {
            hdr.hdrA.f1  : exact;
            meta.metaA.f1: exact;
        }
        size = 128;
    }
    @name(".t_ingress_2") table t_ingress_2 {
        actions = {
            _nop;
            _resubmit;
            _clone_i2e;
        }
        key = {
            hdr.hdrA.f1                    : exact;
            standard_metadata.instance_type: ternary;
        }
        size = 128;
    }
    apply {
        t_ingress_1.apply();
        t_ingress_2.apply();
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit(hdr.hdrA);
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

V1Switch(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;

