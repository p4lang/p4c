#include <core.p4>
#include <v1model.p4>

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

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
    p4calc_t tmp;
    p4calc_t tmp_0;
    p4calc_t tmp_1;
    bit<128> tmp_3;
    bit<128> tmp_4;
    bit<128> tmp_5;
    state start {
        packet.extract<ethernet_t>(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            16w0x1234: check_p4calc;
            default: accept;
        }
    }
    state check_p4calc {
        tmp_3 = packet.lookahead<bit<128>>();
        tmp.setValid();
        tmp.p = tmp_3[127:120];
        tmp.four = tmp_3[119:112];
        tmp.ver = tmp_3[111:104];
        tmp.op = tmp_3[103:96];
        tmp.operand_a = tmp_3[95:64];
        tmp.operand_b = tmp_3[63:32];
        tmp.res = tmp_3[31:0];
        tmp_4 = packet.lookahead<bit<128>>();
        tmp_0.setValid();
        tmp_0.p = tmp_4[127:120];
        tmp_0.four = tmp_4[119:112];
        tmp_0.ver = tmp_4[111:104];
        tmp_0.op = tmp_4[103:96];
        tmp_0.operand_a = tmp_4[95:64];
        tmp_0.operand_b = tmp_4[63:32];
        tmp_0.res = tmp_4[31:0];
        tmp_5 = packet.lookahead<bit<128>>();
        tmp_1.setValid();
        tmp_1.p = tmp_5[127:120];
        tmp_1.four = tmp_5[119:112];
        tmp_1.ver = tmp_5[111:104];
        tmp_1.op = tmp_5[103:96];
        tmp_1.operand_a = tmp_5[95:64];
        tmp_1.operand_b = tmp_5[63:32];
        tmp_1.res = tmp_5[31:0];
        transition select(tmp_3[127:120], tmp_4[119:112], tmp_5[111:104]) {
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

struct tuple_0 {
    bit<32> field;
}

control MyIngress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    bit<48> tmp_2;
    bit<32> nselect_0;
    @name("MyIngress.operation_add") action operation_add() {
        hdr.p4calc.res = hdr.p4calc.operand_a + hdr.p4calc.operand_b;
        tmp_2 = hdr.ethernet.dstAddr;
        hdr.ethernet.dstAddr = hdr.ethernet.srcAddr;
        hdr.ethernet.srcAddr = tmp_2;
        standard_metadata.egress_spec = standard_metadata.ingress_port;
    }
    @name("MyIngress.operation_sub") action operation_sub() {
        hdr.p4calc.res = hdr.p4calc.operand_a - hdr.p4calc.operand_b;
        tmp_2 = hdr.ethernet.dstAddr;
        hdr.ethernet.dstAddr = hdr.ethernet.srcAddr;
        hdr.ethernet.srcAddr = tmp_2;
        standard_metadata.egress_spec = standard_metadata.ingress_port;
    }
    @name("MyIngress.operation_and") action operation_and() {
        hdr.p4calc.res = hdr.p4calc.operand_a & hdr.p4calc.operand_b;
        tmp_2 = hdr.ethernet.dstAddr;
        hdr.ethernet.dstAddr = hdr.ethernet.srcAddr;
        hdr.ethernet.srcAddr = tmp_2;
        standard_metadata.egress_spec = standard_metadata.ingress_port;
    }
    @name("MyIngress.operation_or") action operation_or() {
        hdr.p4calc.res = hdr.p4calc.operand_a | hdr.p4calc.operand_b;
        tmp_2 = hdr.ethernet.dstAddr;
        hdr.ethernet.dstAddr = hdr.ethernet.srcAddr;
        hdr.ethernet.srcAddr = tmp_2;
        standard_metadata.egress_spec = standard_metadata.ingress_port;
    }
    @name("MyIngress.operation_xor") action operation_xor() {
        hdr.p4calc.res = hdr.p4calc.operand_a ^ hdr.p4calc.operand_b;
        tmp_2 = hdr.ethernet.dstAddr;
        hdr.ethernet.dstAddr = hdr.ethernet.srcAddr;
        hdr.ethernet.srcAddr = tmp_2;
        standard_metadata.egress_spec = standard_metadata.ingress_port;
    }
    @name("MyIngress.operation_crc") action operation_crc() {
        hash<bit<32>, bit<32>, tuple_0, bit<64>>(nselect_0, HashAlgorithm.crc32, hdr.p4calc.operand_b, { hdr.p4calc.operand_a }, 64w8589934592);
        hdr.p4calc.res = nselect_0;
        tmp_2 = hdr.ethernet.dstAddr;
        hdr.ethernet.dstAddr = hdr.ethernet.srcAddr;
        hdr.ethernet.srcAddr = tmp_2;
        standard_metadata.egress_spec = standard_metadata.ingress_port;
    }
    @name("MyIngress.operation_drop") action operation_drop() {
        mark_to_drop(standard_metadata);
    }
    @name("MyIngress.operation_drop") action operation_drop_2() {
        mark_to_drop(standard_metadata);
    }
    @name("MyIngress.calculate") table calculate_0 {
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
    @hidden table tbl_operation_drop {
        actions = {
            operation_drop_2();
        }
        const default_action = operation_drop_2();
    }
    apply {
        if (hdr.p4calc.isValid()) {
            calculate_0.apply();
        } else {
            tbl_operation_drop.apply();
        }
    }
}

control MyEgress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control MyComputeChecksum(inout headers hdr, inout metadata meta) {
    apply {
        update_checksum<tuple_0, bit<32>>(hdr.p4calc.isValid(), { hdr.p4calc.operand_a }, hdr.p4calc.res, HashAlgorithm.crc32);
    }
}

control MyDeparser(packet_out packet, in headers hdr) {
    apply {
        packet.emit<ethernet_t>(hdr.ethernet);
        packet.emit<p4calc_t>(hdr.p4calc);
    }
}

V1Switch<headers, metadata>(MyParser(), MyVerifyChecksum(), MyIngress(), MyEgress(), MyComputeChecksum(), MyDeparser()) main;

