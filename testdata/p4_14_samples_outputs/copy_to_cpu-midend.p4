#include <core.p4>
#define V1MODEL_VERSION 20200408
#include <v1model.p4>

struct intrinsic_metadata_t {
    bit<4> mcast_grp;
    bit<4> egress_rid;
}

header cpu_header_t {
    bit<8> device;
    bit<8> reason;
}

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

struct metadata {
}

struct headers {
    @name(".cpu_header") 
    cpu_header_t cpu_header;
    @name(".ethernet") 
    ethernet_t   ethernet;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    bit<64> tmp;
    @name(".parse_cpu_header") state parse_cpu_header {
        packet.extract<cpu_header_t>(hdr.cpu_header);
        transition parse_ethernet;
    }
    @name(".parse_ethernet") state parse_ethernet {
        packet.extract<ethernet_t>(hdr.ethernet);
        transition accept;
    }
    @name(".start") state start {
        tmp = packet.lookahead<bit<64>>();
        transition select(tmp) {
            64w0: parse_cpu_header;
            default: parse_ethernet;
        }
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @noWarn("unused") @name(".NoAction") action NoAction_0() {
    }
    @name("._drop") action _drop() {
        mark_to_drop(standard_metadata);
    }
    @name(".do_cpu_encap") action do_cpu_encap() {
        hdr.cpu_header.setValid();
        hdr.cpu_header.device = 8w0;
        hdr.cpu_header.reason = 8w0xab;
    }
    @name(".redirect") table redirect_0 {
        actions = {
            _drop();
            do_cpu_encap();
            @defaultonly NoAction_0();
        }
        key = {
            standard_metadata.instance_type: exact @name("standard_metadata.instance_type") ;
        }
        size = 16;
        default_action = NoAction_0();
    }
    apply {
        redirect_0.apply();
    }
}

struct tuple_0 {
    standard_metadata_t field;
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name(".do_copy_to_cpu") action do_copy_to_cpu() {
        clone3<tuple_0>(CloneType.I2E, 32w250, { standard_metadata });
    }
    @name(".copy_to_cpu") table copy_to_cpu_0 {
        actions = {
            do_copy_to_cpu();
            @defaultonly NoAction_1();
        }
        size = 1;
        default_action = NoAction_1();
    }
    apply {
        copy_to_cpu_0.apply();
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit<cpu_header_t>(hdr.cpu_header);
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

