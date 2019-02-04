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
    bit<4>  _intrinsic_metadata_mcast_grp0;
    bit<4>  _intrinsic_metadata_egress_rid1;
    bit<16> _intrinsic_metadata_mcast_hash2;
    bit<32> _intrinsic_metadata_lf_field_list3;
    bit<16> _intrinsic_metadata_resubmit_flag4;
    bit<8>  _mymeta_f15;
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
    @name("._nop") action _nop() {
    }
    @name("._nop") action _nop_2() {
    }
    @name(".set_port") action set_port(bit<9> port) {
        standard_metadata.egress_spec = port;
    }
    @name("._resubmit") action _resubmit() {
        meta._mymeta_f15 = 8w1;
        resubmit<tuple_0>({ standard_metadata, {8w1} });
    }
    @name(".t_ingress_1") table t_ingress {
        actions = {
            _nop();
            set_port();
            @defaultonly NoAction_0();
        }
        key = {
            meta._mymeta_f15: exact @name("mymeta.f1") ;
        }
        size = 128;
        default_action = NoAction_0();
    }
    @name(".t_ingress_2") table t_ingress_0 {
        actions = {
            _nop_2();
            _resubmit();
            @defaultonly NoAction_3();
        }
        key = {
            meta._mymeta_f15: exact @name("mymeta.f1") ;
        }
        size = 128;
        default_action = NoAction_3();
    }
    apply {
        t_ingress.apply();
        t_ingress_0.apply();
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

