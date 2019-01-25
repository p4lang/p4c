#include <core.p4>
#include <v1model.p4>

struct intrinsic_metadata_t {
    bit<4>  mcast_grp;
    bit<4>  egress_rid;
    bit<32> lf_field_list;
    bit<16> resubmit_flag;
}

struct mymeta_t {
    @recirculate 
    bit<8> f1;
}

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

struct metadata {
    bit<8> _mymeta_f10;
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
<<<<<<< 43bd696b29be944551572728dfc9ec48437ee961
        meta._mymeta_f10 = 8w1;
=======
<<<<<<< 1968b35515ddd4809e438d338481981969628fc8
        meta._mymeta_f14 = 8w1;
>>>>>>> Tag metadata fields that need to be recirculated
        resubmit<tuple_0>({ standard_metadata, {8w1} });
=======
        meta.mymeta.f1 = 8w1;
        resubmit();
>>>>>>> Tag metadata fields that need to be recirculated
    }
    @name(".t_ingress_1") table t_ingress {
        actions = {
            _nop();
            set_port();
            @defaultonly NoAction_0();
        }
        key = {
            meta._mymeta_f10: exact @name("mymeta.f1") ;
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
            meta._mymeta_f10: exact @name("mymeta.f1") ;
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

