#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

@controller_header("packet_out") header packet_out_t {
    bit<9>  egress_port;
    bit<8>  queue_id;
    bit<48> not_actually_useful;
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

header andycustom_t {
    bit<2>  version;
    bit<6>  dscp;
    bit<16> totalLen;
    bit<8>  ttl;
    bit<8>  protocol;
    bit<8>  l4Offset;
    bit<8>  flags;
    bit<8>  rsvd;
    bit<32> srcAddr;
    bit<32> dstAddr;
}

struct headers_t {
    packet_out_t cpu;
    ethernet_t   ethernet;
    ipv4_t       ipv4;
    andycustom_t andycustom;
}

struct metadata_t {
}

parser parserImpl(packet_in packet, out headers_t hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    state start {
        transition select(stdmeta.ingress_port) {
            9w111: parse_cpu_hdr;
            default: parse_ethernet;
        }
    }
    state parse_cpu_hdr {
        packet.extract<packet_out_t>(hdr.cpu);
        transition parse_ethernet;
    }
    state parse_ethernet {
        packet.extract<ethernet_t>(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            16w0x800: parse_ipv4;
            16w0xd00d: parse_andycustom;
            default: accept;
        }
    }
    state parse_andycustom {
        packet.extract<andycustom_t>(hdr.andycustom);
        transition select(hdr.andycustom.protocol) {
            8w4: parse_ipv4;
            default: accept;
        }
    }
    state parse_ipv4 {
        packet.extract<ipv4_t>(hdr.ipv4);
        transition accept;
    }
}

control verifyChecksum(inout headers_t hdr, inout metadata_t meta) {
    apply {
    }
}

control ingressImpl(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("ingressImpl.my_drop") action my_drop() {
        mark_to_drop(stdmeta);
    }
    @name("ingressImpl.set_addr") action set_addr(@name("new_dstAddr") bit<32> new_dstAddr) {
        hdr.ipv4.dstAddr = new_dstAddr;
        stdmeta.egress_spec = stdmeta.ingress_port;
    }
    @name("ingressImpl.t1") table t1_0 {
        key = {
            hdr.andycustom.srcAddr: exact @name("hdr.andycustom.srcAddr");
        }
        actions = {
            set_addr();
            my_drop();
            NoAction_1();
        }
        const default_action = NoAction_1();
    }
    apply {
        t1_0.apply();
    }
}

control egressImpl(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    apply {
    }
}

control updateChecksum(inout headers_t hdr, inout metadata_t meta) {
    apply {
    }
}

control deparserImpl(packet_out packet, in headers_t hdr) {
    apply {
        packet.emit<ethernet_t>(hdr.ethernet);
    }
}

V1Switch<headers_t, metadata_t>(parserImpl(), verifyChecksum(), ingressImpl(), egressImpl(), updateChecksum(), deparserImpl()) main;
