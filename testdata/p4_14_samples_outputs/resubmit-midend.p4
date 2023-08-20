#include <core.p4>
#define V1MODEL_VERSION 20200408
#include <v1model.p4>

struct intrinsic_metadata_t {
    bit<4> mcast_grp;
    bit<4> egress_rid;
}

struct mymeta_t {
    @field_list(8w1)
    bit<8> f1;
}

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

struct metadata {
    @field_list(8w1)
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
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_2() {
    }
    @name("._nop") action _nop() {
    }
    @name("._nop") action _nop_1() {
    }
    @name(".set_port") action set_port(@name("port") bit<9> port) {
        standard_metadata.egress_spec = port;
    }
    @name("._resubmit") action _resubmit() {
        meta._mymeta_f10 = 8w1;
        resubmit_preserving_field_list(8w1);
    }
    @name(".t_ingress_1") table t_ingress {
        actions = {
            _nop();
            set_port();
            @defaultonly NoAction_1();
        }
        key = {
            meta._mymeta_f10: exact @name("mymeta.f1");
        }
        size = 128;
        default_action = NoAction_1();
    }
    @name(".t_ingress_2") table t_ingress_0 {
        actions = {
            _nop_1();
            _resubmit();
            @defaultonly NoAction_2();
        }
        key = {
            meta._mymeta_f10: exact @name("mymeta.f1");
        }
        size = 128;
        default_action = NoAction_2();
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
