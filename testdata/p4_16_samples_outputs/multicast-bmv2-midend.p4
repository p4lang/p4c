#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

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
    bit<32> _routing_metadata_nhop_ipv40;
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
    @noWarn("unused") @name(".NoAction") action NoAction_2() {
    }
    @name(".rewrite_mac") action rewrite_mac(@name("smac") bit<48> smac) {
        hdr.ethernet.srcAddr = smac;
    }
    @name("._drop") action _drop() {
        mark_to_drop(standard_metadata);
    }
    @name("egress.send_frame") table send_frame_0 {
        actions = {
            rewrite_mac();
            _drop();
            @defaultonly NoAction_2();
        }
        key = {
            standard_metadata.egress_port: exact @name("standard_metadata.egress_port");
        }
        size = 256;
        default_action = NoAction_2();
    }
    apply {
        send_frame_0.apply();
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @noWarn("unused") @name(".NoAction") action NoAction_3() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_4() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_5() {
    }
    @name(".bcast") action bcast() {
        standard_metadata.mcast_grp = 16w1;
    }
    @name(".set_dmac") action set_dmac(@name("dmac") bit<48> dmac) {
        hdr.ethernet.dstAddr = dmac;
    }
    @name("._drop") action _drop_2() {
        mark_to_drop(standard_metadata);
    }
    @name("._drop") action _drop_3() {
        mark_to_drop(standard_metadata);
    }
    @name(".set_nhop") action set_nhop(@name("nhop_ipv4") bit<32> nhop_ipv4_1, @name("port") bit<9> port) {
        meta._routing_metadata_nhop_ipv40 = nhop_ipv4_1;
        standard_metadata.egress_spec = port;
        hdr.ipv4.ttl = hdr.ipv4.ttl + 8w255;
    }
    @name("ingress.broadcast") table broadcast_0 {
        actions = {
            bcast();
            @defaultonly NoAction_3();
        }
        size = 1;
        default_action = NoAction_3();
    }
    @name("ingress.forward") table forward_0 {
        actions = {
            set_dmac();
            _drop_2();
            @defaultonly NoAction_4();
        }
        key = {
            meta._routing_metadata_nhop_ipv40: exact @name("meta.routing_metadata.nhop_ipv4");
        }
        size = 512;
        default_action = NoAction_4();
    }
    @name("ingress.ipv4_lpm") table ipv4_lpm_0 {
        actions = {
            set_nhop();
            _drop_3();
            @defaultonly NoAction_5();
        }
        key = {
            hdr.ipv4.dstAddr: lpm @name("hdr.ipv4.dstAddr");
        }
        size = 1024;
        default_action = NoAction_5();
    }
    apply {
        if (hdr.ipv4.isValid()) {
            ipv4_lpm_0.apply();
            forward_0.apply();
        } else {
            broadcast_0.apply();
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
    }
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

V1Switch<headers, metadata>(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;
