#include <core.p4>
#include <v1model.p4>

struct intrinsic_metadata_t {
    bit<4>  mcast_grp;
    bit<4>  egress_rid;
    bit<16> mcast_hash;
    bit<32> lf_field_list;
    bit<16> resubmit_flag;
}

struct mymeta_t {
    bit<8> f1;
}

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

struct metadata {
    @name(".intrinsic_metadata") 
    intrinsic_metadata_t intrinsic_metadata;
    @name(".mymeta") 
    mymeta_t             mymeta;
}

struct headers {
    @name(".ethernet") 
    ethernet_t ethernet;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".parse_ethernet") state parse_ethernet {
        packet.extract<ethernet_t>(hdr.ethernet);
        transition accept;
    }
    @name(".start") state start {
        transition parse_ethernet;
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

struct tuple_0 {
    standard_metadata_t field;
    mymeta_t            field_0;
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".NoAction") action NoAction_0() {
    }
    @name(".NoAction") action NoAction_3() {
    }
    @name("._nop") action _nop_0() {
    }
    @name("._nop") action _nop_2() {
    }
    @name(".set_port") action set_port_0(bit<9> port) {
        standard_metadata.egress_spec = port;
    }
    @name("._resubmit") action _resubmit_0() {
        meta.mymeta.f1 = 8w1;
        resubmit<tuple_0>({ standard_metadata, meta.mymeta });
    }
    @name(".t_ingress_1") table t_ingress_1 {
        actions = {
            _nop_0();
            set_port_0();
            @defaultonly NoAction_0();
        }
        key = {
            meta.mymeta.f1: exact @name("mymeta.f1") ;
        }
        size = 128;
        default_action = NoAction_0();
    }
    @name(".t_ingress_2") table t_ingress_2 {
        actions = {
            _nop_2();
            _resubmit_0();
            @defaultonly NoAction_3();
        }
        key = {
            meta.mymeta.f1: exact @name("mymeta.f1") ;
        }
        size = 128;
        default_action = NoAction_3();
    }
    apply {
        t_ingress_1.apply();
        t_ingress_2.apply();
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit<ethernet_t>(hdr.ethernet);
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

