#include <core.p4>
#include <v1model.p4>

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

const bit<16> P4CALC_ETYPE = 16w0x1234;
const bit<8> P4CALC_P = 8w0x50;
const bit<8> P4CALC_4 = 8w0x34;
const bit<8> P4CALC_VER = 8w0x1;
const bit<8> P4CALC_PLUS = 8w0x2b;
const bit<8> P4CALC_MINUS = 8w0x2d;
const bit<8> P4CALC_AND = 8w0x26;
const bit<8> P4CALC_OR = 8w0x7c;
const bit<8> P4CALC_CARET = 8w0x5e;
const bit<8> P4CALC_CRC = 8w0x3e;
header p4calc_t {
    bit<8>  p;
    bit<8>  four;
    bit<8>  ver;
    bit<8>  op;
    bit<32> operand_a;
    bit<32> operand_b;
    bit<32> res;
}

struct headers {
    ethernet_t ethernet;
    p4calc_t   p4calc;
}

struct metadata {
}

parser MyParser(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    state start {
        packet.extract<ethernet_t>(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            16w0x1234: check_p4calc;
            default: accept;
        }
    }
    state check_p4calc {
        transition select((packet.lookahead<p4calc_t>()).p, (packet.lookahead<p4calc_t>()).four, (packet.lookahead<p4calc_t>()).ver) {
            (8w0x50, 8w0x34, 8w0x1): parse_p4calc;
            default: accept;
        }
    }
    state parse_p4calc {
        packet.extract<p4calc_t>(hdr.p4calc);
        transition accept;
    }
}

control MyVerifyChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

control MyIngress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    action send_back(bit<32> result) {
        bit<48> tmp;
        hdr.p4calc.res = result;
        tmp = hdr.ethernet.dstAddr;
        hdr.ethernet.dstAddr = hdr.ethernet.srcAddr;
        hdr.ethernet.srcAddr = tmp;
        standard_metadata.egress_spec = standard_metadata.ingress_port;
    }
    action operation_add() {
        send_back(hdr.p4calc.operand_a + hdr.p4calc.operand_b);
    }
    action operation_sub() {
        send_back(hdr.p4calc.operand_a - hdr.p4calc.operand_b);
    }
    action operation_and() {
        send_back(hdr.p4calc.operand_a & hdr.p4calc.operand_b);
    }
    action operation_or() {
        send_back(hdr.p4calc.operand_a | hdr.p4calc.operand_b);
    }
    action operation_xor() {
        send_back(hdr.p4calc.operand_a ^ hdr.p4calc.operand_b);
    }
    action operation_crc() {
        bit<32> nbase = hdr.p4calc.operand_b;
        bit<64> ncount = 64w8589934592;
        bit<32> nselect;
        bit<32> ninput = hdr.p4calc.operand_a;
        hash<bit<32>, bit<32>, tuple<bit<32>>, bit<64>>(nselect, HashAlgorithm.crc32, nbase, { ninput }, ncount);
        send_back(nselect);
    }
    action operation_drop() {
        mark_to_drop();
    }
    table calculate {
        key = {
            hdr.p4calc.op: exact @name("hdr.p4calc.op") ;
        }
        actions = {
            operation_add();
            operation_sub();
            operation_and();
            operation_or();
            operation_xor();
            operation_crc();
            operation_drop();
        }
        const default_action = operation_drop();
        const entries = {
                        8w0x2b : operation_add();

                        8w0x2d : operation_sub();

                        8w0x26 : operation_and();

                        8w0x7c : operation_or();

                        8w0x5e : operation_xor();

                        8w0x3e : operation_crc();

        }

    }
    apply {
        if (hdr.p4calc.isValid()) 
            calculate.apply();
        else 
            operation_drop();
    }
}

control MyEgress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control MyComputeChecksum(inout headers hdr, inout metadata meta) {
    apply {
        update_checksum<tuple<bit<32>>, bit<32>>(hdr.p4calc.isValid(), { hdr.p4calc.operand_a }, hdr.p4calc.res, HashAlgorithm.crc32);
    }
}

control MyDeparser(packet_out packet, in headers hdr) {
    apply {
        packet.emit<ethernet_t>(hdr.ethernet);
        packet.emit<p4calc_t>(hdr.p4calc);
    }
}

V1Switch<headers, metadata>(MyParser(), MyVerifyChecksum(), MyIngress(), MyEgress(), MyComputeChecksum(), MyDeparser()) main;

