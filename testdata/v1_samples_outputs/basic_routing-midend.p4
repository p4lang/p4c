#include "/home/mbudiu/barefoot/git/p4c/build/../p4include/core.p4"
#include "/home/mbudiu/barefoot/git/p4c/build/../p4include/v1model.p4"

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
    @name("ingress_metadata") 
    ingress_metadata_t ingress_metadata;
}

struct headers {
    @name("ethernet") 
    ethernet_t ethernet;
    @name("ipv4") 
    ipv4_t     ipv4;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("parse_ethernet") state parse_ethernet {
        packet.extract(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            16w0x800: parse_ipv4;
            default: accept;
        }
    }
    @name("parse_ipv4") state parse_ipv4 {
        packet.extract(hdr.ipv4);
        transition accept;
    }
    @name("start") state start {
        transition parse_ethernet;
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("on_miss") action on_miss_0() {
    }
    @name("rewrite_src_dst_mac") action rewrite_src_dst_mac_0(bit<48> smac, bit<48> dmac) {
        hdr.ethernet.srcAddr = smac;
        hdr.ethernet.dstAddr = dmac;
    }
    @name("rewrite_mac") table rewrite_mac_0() {
        actions = {
            on_miss_0;
            rewrite_src_dst_mac_0;
            NoAction;
        }
        key = {
            meta.ingress_metadata.nexthop_index: exact;
        }
        size = 32768;
        default_action = NoAction();
    }
    apply {
        rewrite_mac_0.apply();
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("set_vrf") action set_vrf_0(bit<12> vrf) {
        meta.ingress_metadata.vrf = vrf;
    }
    @name("on_miss") action on_miss_1() {
    }
    @name("fib_hit_nexthop") action fib_hit_nexthop_0(bit<16> nexthop_index) {
        meta.ingress_metadata.nexthop_index = nexthop_index;
        hdr.ipv4.ttl = hdr.ipv4.ttl + 8w255;
    }
    @name("set_egress_details") action set_egress_details_0(bit<9> egress_spec) {
        standard_metadata.egress_spec = egress_spec;
    }
    @name("set_bd") action set_bd_0(bit<16> bd) {
        meta.ingress_metadata.bd = bd;
    }
    @name("bd") table bd_0() {
        actions = {
            set_vrf_0;
            NoAction;
        }
        key = {
            meta.ingress_metadata.bd: exact;
        }
        size = 65536;
        default_action = NoAction();
    }
    @name("ipv4_fib") table ipv4_fib_0() {
        actions = {
            on_miss_1;
            fib_hit_nexthop_0;
            NoAction;
        }
        key = {
            meta.ingress_metadata.vrf: exact;
            hdr.ipv4.dstAddr         : exact;
        }
        size = 131072;
        default_action = NoAction();
    }
    @name("ipv4_fib_lpm") table ipv4_fib_lpm_0() {
        actions = {
            on_miss_1;
            fib_hit_nexthop_0;
            NoAction;
        }
        key = {
            meta.ingress_metadata.vrf: exact;
            hdr.ipv4.dstAddr         : lpm;
        }
        size = 16384;
        default_action = NoAction();
    }
    @name("nexthop") table nexthop_0() {
        actions = {
            on_miss_1;
            set_egress_details_0;
            NoAction;
        }
        key = {
            meta.ingress_metadata.nexthop_index: exact;
        }
        size = 32768;
        default_action = NoAction();
    }
    @name("port_mapping") table port_mapping_0() {
        actions = {
            set_bd_0;
            NoAction;
        }
        key = {
            standard_metadata.ingress_port: exact;
        }
        size = 32768;
        default_action = NoAction();
    }
    apply {
        if (hdr.ipv4.isValid()) {
            port_mapping_0.apply();
            bd_0.apply();
            switch (ipv4_fib_0.apply().action_run) {
                on_miss_1: {
                    ipv4_fib_lpm_0.apply();
                }
            }

            nexthop_0.apply();
        }
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit(hdr.ethernet);
        packet.emit(hdr.ipv4);
    }
}

control verifyChecksum(in headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    Checksum16() @name("ipv4_checksum") ipv4_checksum_0;
    action act() {
        standard_metadata.drop = 1w1;
    }
    table tbl_act() {
        actions = {
            act;
        }
        const default_action = act();
    }
    apply {
        if (hdr.ipv4.hdrChecksum == ipv4_checksum_0.get({ hdr.ipv4.version, hdr.ipv4.ihl, hdr.ipv4.diffserv, hdr.ipv4.totalLen, hdr.ipv4.identification, hdr.ipv4.flags, hdr.ipv4.fragOffset, hdr.ipv4.ttl, hdr.ipv4.protocol, hdr.ipv4.srcAddr, hdr.ipv4.dstAddr })) 
            tbl_act.apply();
    }
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    Checksum16() @name("ipv4_checksum") ipv4_checksum_1;
    action act_0() {
        hdr.ipv4.hdrChecksum = ipv4_checksum_1.get({ hdr.ipv4.version, hdr.ipv4.ihl, hdr.ipv4.diffserv, hdr.ipv4.totalLen, hdr.ipv4.identification, hdr.ipv4.flags, hdr.ipv4.fragOffset, hdr.ipv4.ttl, hdr.ipv4.protocol, hdr.ipv4.srcAddr, hdr.ipv4.dstAddr });
    }
    table tbl_act_0() {
        actions = {
            act_0;
        }
        const default_action = act_0();
    }
    apply {
        tbl_act_0.apply();
    }
}

V1Switch(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;
