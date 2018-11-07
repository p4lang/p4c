#include <core.p4>
#include <v1model.p4>

struct egress_metadata_t {
    bit<16> payload_length;
    bit<9>  smac_idx;
    bit<16> bd;
    bit<1>  inner_replica;
    bit<1>  replica;
    bit<48> mac_da;
    bit<1>  routed;
    bit<16> same_bd_check;
    bit<4>  header_count;
    bit<8>  drop_reason;
    bit<1>  egress_bypass;
    bit<8>  drop_exception;
}

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

header ipv6_t {
    bit<4>   version;
    bit<8>   trafficClass;
    bit<20>  flowLabel;
    bit<16>  payloadLen;
    bit<8>   nextHdr;
    bit<8>   hopLimit;
    bit<128> srcAddr;
    bit<128> dstAddr;
}

struct metadata {
    @name(".egress_metadata") 
    egress_metadata_t egress_metadata;
}

struct headers {
    @name(".ethernet") 
    ethernet_t ethernet;
    @name(".ipv4") 
    ipv4_t     ipv4;
    @name(".ipv6") 
    ipv6_t     ipv6;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".parse_ethernet") state parse_ethernet {
        packet.extract<ethernet_t>(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            16w0x800: parse_ipv4;
            16w0x86dd: parse_ipv6;
            default: accept;
        }
    }
    @name(".parse_ipv4") state parse_ipv4 {
        packet.extract<ipv4_t>(hdr.ipv4);
        transition accept;
    }
    @name(".parse_ipv6") state parse_ipv6 {
        packet.extract<ipv6_t>(hdr.ipv6);
        transition accept;
    }
    @name(".start") state start {
        transition parse_ethernet;
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".NoAction") action NoAction_0() {
    }
    @name(".NoAction") action NoAction_3() {
    }
    @name(".do_setup") action do_setup(bit<9> idx, bit<1> routed) {
        meta.egress_metadata.mac_da = hdr.ethernet.dstAddr;
        meta.egress_metadata.smac_idx = idx;
        meta.egress_metadata.routed = routed;
    }
    @name(".setup") table setup_0 {
        actions = {
            do_setup();
            @defaultonly NoAction_0();
        }
        key = {
            hdr.ethernet.isValid(): exact @name("ethernet.$valid$") ;
        }
        default_action = NoAction_0();
    }
    @name(".nop") action _nop_0() {
    }
    @name(".rewrite_ipv4_unicast_mac") action _rewrite_ipv4_unicast_mac_0(bit<48> smac) {
        hdr.ethernet.srcAddr = smac;
        hdr.ethernet.dstAddr = meta.egress_metadata.mac_da;
        hdr.ipv4.ttl = hdr.ipv4.ttl + 8w255;
    }
    @name(".rewrite_ipv4_multicast_mac") action _rewrite_ipv4_multicast_mac_0(bit<48> smac) {
        hdr.ethernet.srcAddr = smac;
        hdr.ethernet.dstAddr[47:23] = 25w0x200bc;
        hdr.ipv4.ttl = hdr.ipv4.ttl + 8w255;
    }
    @name(".rewrite_ipv6_unicast_mac") action _rewrite_ipv6_unicast_mac_0(bit<48> smac) {
        hdr.ethernet.srcAddr = smac;
        hdr.ethernet.dstAddr = meta.egress_metadata.mac_da;
        hdr.ipv6.hopLimit = hdr.ipv6.hopLimit + 8w255;
    }
    @name(".rewrite_ipv6_multicast_mac") action _rewrite_ipv6_multicast_mac_0(bit<48> smac) {
        hdr.ethernet.srcAddr = smac;
        hdr.ethernet.dstAddr[47:32] = 16w0x3333;
        hdr.ipv6.hopLimit = hdr.ipv6.hopLimit + 8w255;
    }
    @name(".mac_rewrite") table _mac_rewrite {
        actions = {
            _nop_0();
            _rewrite_ipv4_unicast_mac_0();
            _rewrite_ipv4_multicast_mac_0();
            _rewrite_ipv6_unicast_mac_0();
            _rewrite_ipv6_multicast_mac_0();
            @defaultonly NoAction_3();
        }
        key = {
            meta.egress_metadata.smac_idx: exact @name("egress_metadata.smac_idx") ;
            hdr.ipv4.isValid()           : exact @name("ipv4.$valid$") ;
            hdr.ipv6.isValid()           : exact @name("ipv6.$valid$") ;
        }
        size = 512;
        default_action = NoAction_3();
    }
    apply {
        setup_0.apply();
        if (meta.egress_metadata.routed == 1w1) 
            _mac_rewrite.apply();
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit<ethernet_t>(hdr.ethernet);
        packet.emit<ipv6_t>(hdr.ipv6);
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

