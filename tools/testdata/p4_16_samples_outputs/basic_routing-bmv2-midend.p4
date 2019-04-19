#include <core.p4>
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
    bit<12> _ingress_metadata_vrf0;
    bit<16> _ingress_metadata_bd1;
    bit<16> _ingress_metadata_nexthop_index2;
}

struct headers {
    ethernet_t ethernet;
    ipv4_t     ipv4;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    state parse_ipv4 {
        packet.extract<ipv4_t>(hdr = hdr.ipv4);
        transition accept;
    }
    state start {
        packet.extract<ethernet_t>(hdr = hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            16w0x800: parse_ipv4;
            default: accept;
        }
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".NoAction") action NoAction_0() {
    }
    @name("egress.on_miss") action on_miss() {
    }
    @name("egress.rewrite_src_dst_mac") action rewrite_src_dst_mac(bit<48> smac, bit<48> dmac) {
        hdr.ethernet.srcAddr = smac;
        hdr.ethernet.dstAddr = dmac;
    }
    @name("egress.rewrite_mac") table rewrite_mac_0 {
        actions = {
            on_miss();
            rewrite_src_dst_mac();
            @defaultonly NoAction_0();
        }
        key = {
            meta._ingress_metadata_nexthop_index2: exact @name("meta.ingress_metadata.nexthop_index") ;
        }
        size = 32768;
        default_action = NoAction_0();
    }
    apply {
        rewrite_mac_0.apply();
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".NoAction") action NoAction_1() {
    }
    @name(".NoAction") action NoAction_8() {
    }
    @name(".NoAction") action NoAction_9() {
    }
    @name(".NoAction") action NoAction_10() {
    }
    @name(".NoAction") action NoAction_11() {
    }
    @name("ingress.set_vrf") action set_vrf(bit<12> vrf) {
        meta._ingress_metadata_vrf0 = vrf;
    }
    @name("ingress.on_miss") action on_miss_2() {
    }
    @name("ingress.on_miss") action on_miss_5() {
    }
    @name("ingress.on_miss") action on_miss_6() {
    }
    @name("ingress.fib_hit_nexthop") action fib_hit_nexthop(bit<16> nexthop_index) {
        meta._ingress_metadata_nexthop_index2 = nexthop_index;
        hdr.ipv4.ttl = hdr.ipv4.ttl + 8w255;
    }
    @name("ingress.fib_hit_nexthop") action fib_hit_nexthop_2(bit<16> nexthop_index) {
        meta._ingress_metadata_nexthop_index2 = nexthop_index;
        hdr.ipv4.ttl = hdr.ipv4.ttl + 8w255;
    }
    @name("ingress.set_egress_details") action set_egress_details(bit<9> egress_spec) {
        standard_metadata.egress_spec = egress_spec;
    }
    @name("ingress.set_bd") action set_bd(bit<16> bd) {
        meta._ingress_metadata_bd1 = bd;
    }
    @name("ingress.bd") table bd_0 {
        actions = {
            set_vrf();
            @defaultonly NoAction_1();
        }
        key = {
            meta._ingress_metadata_bd1: exact @name("meta.ingress_metadata.bd") ;
        }
        size = 65536;
        default_action = NoAction_1();
    }
    @name("ingress.ipv4_fib") table ipv4_fib_0 {
        actions = {
            on_miss_2();
            fib_hit_nexthop();
            @defaultonly NoAction_8();
        }
        key = {
            meta._ingress_metadata_vrf0: exact @name("meta.ingress_metadata.vrf") ;
            hdr.ipv4.dstAddr           : exact @name("hdr.ipv4.dstAddr") ;
        }
        size = 131072;
        default_action = NoAction_8();
    }
    @name("ingress.ipv4_fib_lpm") table ipv4_fib_lpm_0 {
        actions = {
            on_miss_5();
            fib_hit_nexthop_2();
            @defaultonly NoAction_9();
        }
        key = {
            meta._ingress_metadata_vrf0: exact @name("meta.ingress_metadata.vrf") ;
            hdr.ipv4.dstAddr           : lpm @name("hdr.ipv4.dstAddr") ;
        }
        size = 16384;
        default_action = NoAction_9();
    }
    @name("ingress.nexthop") table nexthop_0 {
        actions = {
            on_miss_6();
            set_egress_details();
            @defaultonly NoAction_10();
        }
        key = {
            meta._ingress_metadata_nexthop_index2: exact @name("meta.ingress_metadata.nexthop_index") ;
        }
        size = 32768;
        default_action = NoAction_10();
    }
    @name("ingress.port_mapping") table port_mapping_0 {
        actions = {
            set_bd();
            @defaultonly NoAction_11();
        }
        key = {
            standard_metadata.ingress_port: exact @name("standard_metadata.ingress_port") ;
        }
        size = 32768;
        default_action = NoAction_11();
    }
    apply {
        if (hdr.ipv4.isValid()) {
            port_mapping_0.apply();
            bd_0.apply();
            switch (ipv4_fib_0.apply().action_run) {
                on_miss_2: {
                    ipv4_fib_lpm_0.apply();
                }
            }

            nexthop_0.apply();
        }
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit<ethernet_t>(hdr = hdr.ethernet);
        packet.emit<ipv4_t>(hdr = hdr.ipv4);
    }
}

struct tuple_0 {
    bit<4>  field;
    bit<4>  field_0;
    bit<8>  field_1;
    bit<16> field_2;
    bit<16> field_3;
    bit<3>  field_4;
    bit<13> field_5;
    bit<8>  field_6;
    bit<8>  field_7;
    bit<32> field_8;
    bit<32> field_9;
}

control verifyChecksum(inout headers hdr, inout metadata meta) {
    apply {
        verify_checksum<tuple_0, bit<16>>(data = { hdr.ipv4.version, hdr.ipv4.ihl, hdr.ipv4.diffserv, hdr.ipv4.totalLen, hdr.ipv4.identification, hdr.ipv4.flags, hdr.ipv4.fragOffset, hdr.ipv4.ttl, hdr.ipv4.protocol, hdr.ipv4.srcAddr, hdr.ipv4.dstAddr }, checksum = hdr.ipv4.hdrChecksum, condition = true, algo = HashAlgorithm.csum16);
    }
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    apply {
        update_checksum<tuple_0, bit<16>>(condition = true, data = { hdr.ipv4.version, hdr.ipv4.ihl, hdr.ipv4.diffserv, hdr.ipv4.totalLen, hdr.ipv4.identification, hdr.ipv4.flags, hdr.ipv4.fragOffset, hdr.ipv4.ttl, hdr.ipv4.protocol, hdr.ipv4.srcAddr, hdr.ipv4.dstAddr }, algo = HashAlgorithm.csum16, checksum = hdr.ipv4.hdrChecksum);
    }
}

V1Switch<headers, metadata>(p = ParserImpl(), ig = ingress(), vr = verifyChecksum(), eg = egress(), ck = computeChecksum(), dep = DeparserImpl()) main;

