#include <v1model.p4>
#include <core.p4>

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

struct metadata {}

struct headers {
    ethernet_t ethernet;
    ipv4_t     ipv4;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    state start {
        transition parse_ethernet;
    }
    state parse_ethernet {
        packet.extract(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            16w0x800: parse_ipv4;
            default: accept;
        }
    }
    state parse_ipv4 {
        packet.extract(hdr.ipv4);
        transition select(hdr.ipv4.version) {
            4: check_ipv4_ihl;
            default: accept;
        }
    }
    state check_ipv4_ihl {
        transition select(hdr.ipv4.ihl) {
            5: check_ipv4_frag_offset;
            default: accept;
        }
    }
    state check_ipv4_frag_offset {
        transition select(hdr.ipv4.fragOffset) {
            // No error for trying to match on values 0 or 0xff
            //0: check_ipv4_protocol;
            //0xff: check_ipv4_protocol;

            // Trying to use either of the next 2 lines gives error
            // from 2017-Sep-15 version of p4c-bm2-ss:
            //     In file: /home/jafinger/p4c/backends/bmv2/helpers.cpp:82
            //     Compiler Bug: Cannot represent 8191 on 1 bytes

            //13w0x1fff: check_ipv4_protocol;
            0x1fff: check_ipv4_protocol;

            // Trying to match on 256 gives error:
            //     Compiler Bug: Cannot represent 256 on 1 bytes
            //256: check_ipv4_protocol;

            default: accept;
        }
    }
    state check_ipv4_protocol {
        transition accept;
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {}
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {}
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {}
}

control verifyChecksum(inout headers hdr, inout metadata meta) {
    apply {}
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    apply {}
}

V1Switch(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;
