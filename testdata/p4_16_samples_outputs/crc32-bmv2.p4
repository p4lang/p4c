#include <core.p4>
#include <v1model.p4>

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

const bit<16> P4CALC_ETYPE = 0x1234;
const bit<8> P4CALC_P = 0x50;
const bit<8> P4CALC_4 = 0x34;
const bit<8> P4CALC_VER = 0x1;
const bit<8> P4CALC_PLUS = 0x2b;
const bit<8> P4CALC_MINUS = 0x2d;
const bit<8> P4CALC_AND = 0x26;
const bit<8> P4CALC_OR = 0x7c;
const bit<8> P4CALC_CARET = 0x5e;
const bit<8> P4CALC_CRC = 0x3e;
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
        packet.extract(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            P4CALC_ETYPE: check_p4calc;
            default: accept;
        }
    }
    state check_p4calc {
        transition select((packet.lookahead<p4calc_t>()).p, (packet.lookahead<p4calc_t>()).four, (packet.lookahead<p4calc_t>()).ver) {
            (P4CALC_P, P4CALC_4, P4CALC_VER): parse_p4calc;
            default: accept;
        }
    }
    state parse_p4calc {
        packet.extract(hdr.p4calc);
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
        bit<64> ncount = 4294967296 * 2;
        bit<32> nselect;
        bit<32> ninput = hdr.p4calc.operand_a;
        hash(nselect, HashAlgorithm.crc32, nbase, { ninput }, ncount);
        send_back(nselect);
    }
    action operation_drop() {
        mark_to_drop(standard_metadata);
    }
    table calculate {
        key = {
            hdr.p4calc.op: exact;
        }
        actions = {
            operation_add;
            operation_sub;
            operation_and;
            operation_or;
            operation_xor;
            operation_crc;
            operation_drop;
        }
        const default_action = operation_drop();
        const entries = {
                        P4CALC_PLUS : operation_add();

                        P4CALC_MINUS : operation_sub();

                        P4CALC_AND : operation_and();

                        P4CALC_OR : operation_or();

                        P4CALC_CARET : operation_xor();

                        P4CALC_CRC : operation_crc();

        }

    }
    apply {
        if (hdr.p4calc.isValid()) {
            calculate.apply();
        } else {
            operation_drop();
        }
    }
}

control MyEgress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control MyComputeChecksum(inout headers hdr, inout metadata meta) {
    apply {
        update_checksum(hdr.p4calc.isValid(), { hdr.p4calc.operand_a }, hdr.p4calc.res, HashAlgorithm.crc32);
    }
}

control MyDeparser(packet_out packet, in headers hdr) {
    apply {
        packet.emit(hdr.ethernet);
        packet.emit(hdr.p4calc);
    }
}

V1Switch(MyParser(), MyVerifyChecksum(), MyIngress(), MyEgress(), MyComputeChecksum(), MyDeparser()) main;

