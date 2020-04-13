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
    @name(".parse_cpu_header") state parse_cpu_header {
        packet.extract<cpu_header_t>(hdr.cpu_header);
        transition parse_ethernet;
    }
    @name(".parse_ethernet") state parse_ethernet {
        packet.extract<ethernet_t>(hdr.ethernet);
        transition accept;
    }
    @name(".start") state start {
        transition select(packet.lookahead<bit<64>>()) {
            64w0: parse_cpu_header;
            default: parse_ethernet;
        }
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("._drop") action _drop() {
        mark_to_drop(standard_metadata);
    }
    @name(".do_cpu_encap") action do_cpu_encap() {
        hdr.cpu_header.setValid();
        hdr.cpu_header.device = 8w0;
        hdr.cpu_header.reason = 8w0xab;
    }
    @name(".redirect") table redirect {
        actions = {
            _drop();
            do_cpu_encap();
            @defaultonly NoAction();
        }
        key = {
            standard_metadata.instance_type: exact @name("standard_metadata.instance_type") ;
        }
        size = 16;
        default_action = NoAction();
    }
    apply {
        redirect.apply();
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".do_copy_to_cpu") action do_copy_to_cpu() {
        clone3<tuple<standard_metadata_t>>(CloneType.I2E, 32w250, { standard_metadata });
    }
    @name(".copy_to_cpu") table copy_to_cpu {
        actions = {
            do_copy_to_cpu();
            @defaultonly NoAction();
        }
        size = 1;
        default_action = NoAction();
    }
    apply {
        copy_to_cpu.apply();
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

