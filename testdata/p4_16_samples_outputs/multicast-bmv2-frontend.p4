#include <core.p4>
#include <v1model.p4>

struct intrinsic_metadata_t {
    bit<16> mcast_grp;
    bit<16> egress_rid;
    bit<16> mcast_hash;
    bit<32> lf_field_list;
    bit<48> ingress_global_timestamp;
}

struct routing_metadata_t {
    bit<32> nhop_ipv4;
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
    @name("intrinsic_metadata") 
    intrinsic_metadata_t intrinsic_metadata;
    @name("routing_metadata") 
    routing_metadata_t   routing_metadata;
}

struct headers {
    @name("ethernet") 
    ethernet_t ethernet;
    @name("ipv4") 
    ipv4_t     ipv4;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("ParserImpl.parse_ethernet") state parse_ethernet {
        packet.extract<ethernet_t>(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            16w0x800: parse_ipv4;
            default: accept;
        }
    }
    @name("ParserImpl.parse_ipv4") state parse_ipv4 {
        packet.extract<ipv4_t>(hdr.ipv4);
        transition accept;
    }
    @name("ParserImpl.start") state start {
        transition parse_ethernet;
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".NoAction") action NoAction_0() {
    }
    @name(".rewrite_mac") action rewrite_mac(bit<48> smac) {
        hdr.ethernet.srcAddr = smac;
    }
    @name("._drop") action _drop() {
        mark_to_drop();
    }
    @name("egress.send_frame") table send_frame_0 {
        actions = {
            rewrite_mac();
            _drop();
            @defaultonly NoAction_0();
        }
        key = {
            standard_metadata.egress_port: exact @name("standard_metadata.egress_port") ;
        }
        size = 256;
        default_action = NoAction_0();
    }
    apply {
        send_frame_0.apply();
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".NoAction") action NoAction_1() {
    }
    @name(".NoAction") action NoAction_6() {
    }
    @name(".NoAction") action NoAction_7() {
    }
    @name(".bcast") action bcast() {
        meta.intrinsic_metadata.mcast_grp = 16w1;
    }
    @name(".set_dmac") action set_dmac(bit<48> dmac) {
        hdr.ethernet.dstAddr = dmac;
    }
    @name("._drop") action _drop_2() {
        mark_to_drop();
    }
    @name("._drop") action _drop_4() {
        mark_to_drop();
    }
    @name(".set_nhop") action set_nhop(bit<32> nhop_ipv4, bit<9> port) {
        meta.routing_metadata.nhop_ipv4 = nhop_ipv4;
        standard_metadata.egress_spec = port;
        hdr.ipv4.ttl = hdr.ipv4.ttl + 8w255;
    }
    @name("ingress.broadcast") table broadcast_0 {
        actions = {
            bcast();
            @defaultonly NoAction_1();
        }
        size = 1;
        default_action = NoAction_1();
    }
    @name("ingress.forward") table forward_0 {
        actions = {
            set_dmac();
            _drop_2();
            @defaultonly NoAction_6();
        }
        key = {
            meta.routing_metadata.nhop_ipv4: exact @name("meta.routing_metadata.nhop_ipv4") ;
        }
        size = 512;
        default_action = NoAction_6();
    }
    @name("ingress.ipv4_lpm") table ipv4_lpm_0 {
        actions = {
            set_nhop();
            _drop_4();
            @defaultonly NoAction_7();
        }
        key = {
            hdr.ipv4.dstAddr: lpm @name("hdr.ipv4.dstAddr") ;
        }
        size = 1024;
        default_action = NoAction_7();
    }
    apply {
        if (hdr.ipv4.isValid()) {
            ipv4_lpm_0.apply();
            forward_0.apply();
        }
        else 
            broadcast_0.apply();
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
    }
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

V1Switch<headers, metadata>(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;

