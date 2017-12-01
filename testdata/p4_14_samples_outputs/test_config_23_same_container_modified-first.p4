#include <core.p4>
#include <v1model.p4>

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

header ipv4_t {
    bit<4>  version;
    bit<4>  ihl;
    bit<8>  diffserv;
    bit<16> totalLen;
    bit<16> identification;
    bit<3>  flags;
    bit<13> fragOffset;
    bit<8>  ttl;
    bit<8>  protocol;
    bit<16> hdrChecksum;
    bit<32> srcAddr;
    bit<32> dstAddr;
}

header vlan_t {
    bit<3>  pcp;
    bit<1>  cfi;
    bit<12> vid;
    bit<16> etherType;
}

struct metadata {
}

struct headers {
    @name(".ethernet") 
    ethernet_t ethernet;
    @name(".ipv4") 
    ipv4_t     ipv4;
    @name(".vlan") 
    vlan_t     vlan;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".parse_ethernet") state parse_ethernet {
        packet.extract<ethernet_t>(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            16w0x800: parse_ipv4;
            16w0x8100: parse_vlan;
            default: accept;
        }
    }
    @name(".parse_ipv4") state parse_ipv4 {
        packet.extract<ipv4_t>(hdr.ipv4);
        transition accept;
    }
    @name(".parse_vlan") state parse_vlan {
        packet.extract<vlan_t>(hdr.vlan);
        transition accept;
    }
    @name(".start") state start {
        transition parse_ethernet;
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".action_0") action action_0(bit<8> my_param0, bit<8> my_param1) {
        hdr.ipv4.protocol[7:3] = my_param0[7:3];
        hdr.ipv4.ttl = my_param1;
    }
    @name(".action_1") action action_1(bit<8> my_param2) {
        hdr.ipv4.totalLen = hdr.ipv4.totalLen;
    }
    @name(".table_0") table table_0 {
        actions = {
            action_0();
            action_1();
            @defaultonly NoAction();
        }
        key = {
            hdr.ipv4.srcAddr: exact @name("ipv4.srcAddr") ;
        }
        max_size = 4096;
        default_action = NoAction();
    }
    apply {
        table_0.apply();
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit<ethernet_t>(hdr.ethernet);
        packet.emit<vlan_t>(hdr.vlan);
        packet.emit<ipv4_t>(hdr.ipv4);
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

