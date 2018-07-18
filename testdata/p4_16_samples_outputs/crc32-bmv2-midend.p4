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
    p4calc_t tmp_3;
    p4calc_t tmp_4;
    p4calc_t tmp_5;
    bit<128> tmp;
    bit<128> tmp_0;
    bit<128> tmp_1;
    state start {
        packet.extract<ethernet_t>(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            16w0x1234: check_p4calc;
            default: accept;
        }
    }
    state check_p4calc {
        tmp = packet.lookahead<bit<128>>();
        tmp_3.setValid();
        tmp_3.p = tmp[127:120];
        tmp_3.four = tmp[119:112];
        tmp_3.ver = tmp[111:104];
        tmp_3.op = tmp[103:96];
        tmp_3.operand_a = tmp[95:64];
        tmp_3.operand_b = tmp[63:32];
        tmp_3.res = tmp[31:0];
        tmp_0 = packet.lookahead<bit<128>>();
        tmp_4.setValid();
        tmp_4.p = tmp_0[127:120];
        tmp_4.four = tmp_0[119:112];
        tmp_4.ver = tmp_0[111:104];
        tmp_4.op = tmp_0[103:96];
        tmp_4.operand_a = tmp_0[95:64];
        tmp_4.operand_b = tmp_0[63:32];
        tmp_4.res = tmp_0[31:0];
        tmp_1 = packet.lookahead<bit<128>>();
        tmp_5.setValid();
        tmp_5.p = tmp_1[127:120];
        tmp_5.four = tmp_1[119:112];
        tmp_5.ver = tmp_1[111:104];
        tmp_5.op = tmp_1[103:96];
        tmp_5.operand_a = tmp_1[95:64];
        tmp_5.operand_b = tmp_1[63:32];
        tmp_5.res = tmp_1[31:0];
        transition select(tmp_3.p, tmp_4.four, tmp_5.ver) {
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
    bit<48> tmp_6;
    bit<32> nselect;
    @name("MyIngress.operation_add") action operation_add_0() {
        hdr.p4calc.res = hdr.p4calc.operand_a + hdr.p4calc.operand_b;
        tmp_6 = hdr.ethernet.dstAddr;
        hdr.ethernet.dstAddr = hdr.ethernet.srcAddr;
        hdr.ethernet.srcAddr = tmp_6;
        standard_metadata.egress_spec = standard_metadata.ingress_port;
    }
    @name("MyIngress.operation_sub") action operation_sub_0() {
        hdr.p4calc.res = hdr.p4calc.operand_a - hdr.p4calc.operand_b;
        tmp_6 = hdr.ethernet.dstAddr;
        hdr.ethernet.dstAddr = hdr.ethernet.srcAddr;
        hdr.ethernet.srcAddr = tmp_6;
        standard_metadata.egress_spec = standard_metadata.ingress_port;
    }
    @name("MyIngress.operation_and") action operation_and_0() {
        hdr.p4calc.res = hdr.p4calc.operand_a & hdr.p4calc.operand_b;
        tmp_6 = hdr.ethernet.dstAddr;
        hdr.ethernet.dstAddr = hdr.ethernet.srcAddr;
        hdr.ethernet.srcAddr = tmp_6;
        standard_metadata.egress_spec = standard_metadata.ingress_port;
    }
    @name("MyIngress.operation_or") action operation_or_0() {
        hdr.p4calc.res = hdr.p4calc.operand_a | hdr.p4calc.operand_b;
        tmp_6 = hdr.ethernet.dstAddr;
        hdr.ethernet.dstAddr = hdr.ethernet.srcAddr;
        hdr.ethernet.srcAddr = tmp_6;
        standard_metadata.egress_spec = standard_metadata.ingress_port;
    }
    @name("MyIngress.operation_xor") action operation_xor_0() {
        hdr.p4calc.res = hdr.p4calc.operand_a ^ hdr.p4calc.operand_b;
        tmp_6 = hdr.ethernet.dstAddr;
        hdr.ethernet.dstAddr = hdr.ethernet.srcAddr;
        hdr.ethernet.srcAddr = tmp_6;
        standard_metadata.egress_spec = standard_metadata.ingress_port;
    }
    @name("MyIngress.operation_crc") action operation_crc_0() {
        hash<bit<32>, bit<32>, tuple_0, bit<64>>(nselect, HashAlgorithm.crc32, hdr.p4calc.operand_b, { hdr.p4calc.operand_a }, 64w8589934592);
        hdr.p4calc.res = nselect;
        tmp_6 = hdr.ethernet.dstAddr;
        hdr.ethernet.dstAddr = hdr.ethernet.srcAddr;
        hdr.ethernet.srcAddr = tmp_6;
        standard_metadata.egress_spec = standard_metadata.ingress_port;
    }
    @name("MyIngress.operation_drop") action operation_drop_0() {
        mark_to_drop();
    }
    @name("MyIngress.operation_drop") action operation_drop_2() {
        mark_to_drop();
    }
    @name("MyIngress.calculate") table calculate {
        key = {
            hdr.p4calc.op: exact @name("hdr.p4calc.op") ;
        }
        actions = {
            operation_add_0();
            operation_sub_0();
            operation_and_0();
            operation_or_0();
            operation_xor_0();
            operation_crc_0();
            operation_drop_0();
        }
        const default_action = operation_drop_0();
        const entries = {
                        8w0x2b : operation_add_0();

                        8w0x2d : operation_sub_0();

                        8w0x26 : operation_and_0();

                        8w0x7c : operation_or_0();

                        8w0x5e : operation_xor_0();

                        8w0x3e : operation_crc_0();

        }

    }
    @hidden table tbl_operation_drop {
        actions = {
            operation_drop_2();
        }
        const default_action = operation_drop_2();
    }
    apply {
        if (hdr.p4calc.isValid()) 
            calculate.apply();
        else 
            tbl_operation_drop.apply();
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

