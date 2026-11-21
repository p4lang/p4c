#include <core.p4>
#define V1MODEL_VERSION 20200408
#include <v1model.p4>

struct ingress_metadata_t {
    bit<12> vrf;
    bit<16> bd;
    bit<16> nexthop_index;
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

struct metadata {
    @name(".ingress_metadata")
    ingress_metadata_t ingress_metadata;
}

struct headers {
    @name(".ethernet")
    ethernet_t ethernet;
    @name(".ipv4")
    ipv4_t     ipv4;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".parse_ethernet") state parse_ethernet {
        packet.extract<ethernet_t>(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            16w0x800: parse_ipv4;
            default: accept;
        }
    }
    @name(".parse_ipv4") state parse_ipv4 {
        packet.extract<ipv4_t>(hdr.ipv4);
        transition accept;
    }
    @name(".start") state start {
        transition parse_ethernet;
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @noWarn("unused") @name(".NoAction") action NoAction_2() {
    }
    @name(".on_miss") action on_miss() {
    }
    @name(".rewrite_src_dst_mac") action rewrite_src_dst_mac(@name("smac") bit<48> smac, @name("dmac") bit<48> dmac) {
        hdr.ethernet.srcAddr = smac;
        hdr.ethernet.dstAddr = dmac;
    }
    @name(".rewrite_mac") table rewrite_mac_0 {
        actions = {
            on_miss();
            rewrite_src_dst_mac();
            @defaultonly NoAction_2();
        }
        key = {
            meta.ingress_metadata.nexthop_index: exact @name("ingress_metadata.nexthop_index");
        }
        size = 32768;
        default_action = NoAction_2();
    }
    apply {
        rewrite_mac_0.apply();
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @noWarn("unused") @name(".NoAction") action NoAction_3() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_4() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_5() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_6() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_7() {
    }
    @name(".set_vrf") action set_vrf(@name("vrf") bit<12> vrf_1) {
        meta.ingress_metadata.vrf = vrf_1;
    }
    @name(".on_miss") action on_miss_2() {
    }
    @name(".on_miss") action on_miss_3() {
    }
    @name(".on_miss") action on_miss_4() {
    }
    @name(".fib_hit_nexthop") action fib_hit_nexthop(@name("nexthop_index") bit<16> nexthop_index_1) {
        meta.ingress_metadata.nexthop_index = nexthop_index_1;
        hdr.ipv4.ttl = hdr.ipv4.ttl + 8w255;
    }
    @name(".fib_hit_nexthop") action fib_hit_nexthop_1(@name("nexthop_index") bit<16> nexthop_index_2) {
        meta.ingress_metadata.nexthop_index = nexthop_index_2;
        hdr.ipv4.ttl = hdr.ipv4.ttl + 8w255;
    }
    @name(".set_egress_details") action set_egress_details(@name("egress_spec") bit<9> egress_spec_1) {
        standard_metadata.egress_spec = egress_spec_1;
    }
    @name(".set_bd") action set_bd(@name("bd") bit<16> bd_0) {
        meta.ingress_metadata.bd = bd_0;
    }
    @name(".bd") table bd_1 {
        actions = {
            set_vrf();
            @defaultonly NoAction_3();
        }
        key = {
            meta.ingress_metadata.bd: exact @name("ingress_metadata.bd");
        }
        size = 65536;
        default_action = NoAction_3();
    }
    @name(".ipv4_fib") table ipv4_fib_0 {
        actions = {
            on_miss_2();
            fib_hit_nexthop();
            @defaultonly NoAction_4();
        }
        key = {
            meta.ingress_metadata.vrf: exact @name("ingress_metadata.vrf");
            hdr.ipv4.dstAddr         : exact @name("ipv4.dstAddr");
        }
        size = 131072;
        default_action = NoAction_4();
    }
    @name(".ipv4_fib_lpm") table ipv4_fib_lpm_0 {
        actions = {
            on_miss_3();
            fib_hit_nexthop_1();
            @defaultonly NoAction_5();
        }
        key = {
            meta.ingress_metadata.vrf: exact @name("ingress_metadata.vrf");
            hdr.ipv4.dstAddr         : lpm @name("ipv4.dstAddr");
        }
        size = 16384;
        default_action = NoAction_5();
    }
    @name(".nexthop") table nexthop_0 {
        actions = {
            on_miss_4();
            set_egress_details();
            @defaultonly NoAction_6();
        }
        key = {
            meta.ingress_metadata.nexthop_index: exact @name("ingress_metadata.nexthop_index");
        }
        size = 32768;
        default_action = NoAction_6();
    }
    @name(".port_mapping") table port_mapping_0 {
        actions = {
            set_bd();
            @defaultonly NoAction_7();
        }
        key = {
            standard_metadata.ingress_port: exact @name("standard_metadata.ingress_port");
        }
        size = 32768;
        default_action = NoAction_7();
    }
    apply {
        if (hdr.ipv4.isValid()) {
            port_mapping_0.apply();
            bd_1.apply();
            switch (ipv4_fib_0.apply().action_run) {
                on_miss_2: {
                    ipv4_fib_lpm_0.apply();
                }
                default: {
                }
            }
            nexthop_0.apply();
        }
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit<ethernet_t>(hdr.ethernet);
        packet.emit<ipv4_t>(hdr.ipv4);
    }
}

control verifyChecksum(inout headers hdr, inout metadata meta) {
    apply {
        verify_checksum<tuple<bit<4>, bit<4>, bit<8>, bit<16>, bit<16>, bit<3>, bit<13>, bit<8>, bit<8>, bit<32>, bit<32>>, bit<16>>(true, { hdr.ipv4.version, hdr.ipv4.ihl, hdr.ipv4.diffserv, hdr.ipv4.totalLen, hdr.ipv4.identification, hdr.ipv4.flags, hdr.ipv4.fragOffset, hdr.ipv4.ttl, hdr.ipv4.protocol, hdr.ipv4.srcAddr, hdr.ipv4.dstAddr }, hdr.ipv4.hdrChecksum, HashAlgorithm.csum16);
    }
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    apply {
        update_checksum<tuple<bit<4>, bit<4>, bit<8>, bit<16>, bit<16>, bit<3>, bit<13>, bit<8>, bit<8>, bit<32>, bit<32>>, bit<16>>(true, { hdr.ipv4.version, hdr.ipv4.ihl, hdr.ipv4.diffserv, hdr.ipv4.totalLen, hdr.ipv4.identification, hdr.ipv4.flags, hdr.ipv4.fragOffset, hdr.ipv4.ttl, hdr.ipv4.protocol, hdr.ipv4.srcAddr, hdr.ipv4.dstAddr }, hdr.ipv4.hdrChecksum, HashAlgorithm.csum16);
    }
}

V1Switch<headers, metadata>(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;
