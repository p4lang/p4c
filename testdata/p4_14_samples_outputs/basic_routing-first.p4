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
    @name(".on_miss") action on_miss() {
    }
    @name(".rewrite_src_dst_mac") action rewrite_src_dst_mac(bit<48> smac, bit<48> dmac) {
        hdr.ethernet.srcAddr = smac;
        hdr.ethernet.dstAddr = dmac;
    }
    @name(".rewrite_mac") table rewrite_mac {
        actions = {
            on_miss();
            rewrite_src_dst_mac();
            @defaultonly NoAction();
        }
        key = {
            meta.ingress_metadata.nexthop_index: exact @name("ingress_metadata.nexthop_index") ;
        }
        size = 32768;
        default_action = NoAction();
    }
    apply {
        rewrite_mac.apply();
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".set_vrf") action set_vrf(bit<12> vrf) {
        meta.ingress_metadata.vrf = vrf;
    }
    @name(".on_miss") action on_miss() {
    }
    @name(".fib_hit_nexthop") action fib_hit_nexthop(bit<16> nexthop_index) {
        meta.ingress_metadata.nexthop_index = nexthop_index;
        hdr.ipv4.ttl = hdr.ipv4.ttl + 8w255;
    }
    @name(".set_egress_details") action set_egress_details(bit<9> egress_spec) {
        standard_metadata.egress_spec = egress_spec;
    }
    @name(".set_bd") action set_bd(bit<16> bd) {
        meta.ingress_metadata.bd = bd;
    }
    @name(".bd") table bd {
        actions = {
            set_vrf();
            @defaultonly NoAction();
        }
        key = {
            meta.ingress_metadata.bd: exact @name("ingress_metadata.bd") ;
        }
        size = 65536;
        default_action = NoAction();
    }
    @name(".ipv4_fib") table ipv4_fib {
        actions = {
            on_miss();
            fib_hit_nexthop();
            @defaultonly NoAction();
        }
        key = {
            meta.ingress_metadata.vrf: exact @name("ingress_metadata.vrf") ;
            hdr.ipv4.dstAddr         : exact @name("ipv4.dstAddr") ;
        }
        size = 131072;
        default_action = NoAction();
    }
    @name(".ipv4_fib_lpm") table ipv4_fib_lpm {
        actions = {
            on_miss();
            fib_hit_nexthop();
            @defaultonly NoAction();
        }
        key = {
            meta.ingress_metadata.vrf: exact @name("ingress_metadata.vrf") ;
            hdr.ipv4.dstAddr         : lpm @name("ipv4.dstAddr") ;
        }
        size = 16384;
        default_action = NoAction();
    }
    @name(".nexthop") table nexthop {
        actions = {
            on_miss();
            set_egress_details();
            @defaultonly NoAction();
        }
        key = {
            meta.ingress_metadata.nexthop_index: exact @name("ingress_metadata.nexthop_index") ;
        }
        size = 32768;
        default_action = NoAction();
    }
    @name(".port_mapping") table port_mapping {
        actions = {
            set_bd();
            @defaultonly NoAction();
        }
        key = {
            standard_metadata.ingress_port: exact @name("standard_metadata.ingress_port") ;
        }
        size = 32768;
        default_action = NoAction();
    }
    apply {
        if (hdr.ipv4.isValid()) {
            port_mapping.apply();
            bd.apply();
            switch (ipv4_fib.apply().action_run) {
                on_miss: {
                    ipv4_fib_lpm.apply();
                }
            }

            nexthop.apply();
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

