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
    state start {
        packet.extract<ethernet_t>(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            16w0x1234: check_p4calc;
            default: accept;
        }
    }
    state check_p4calc {
        tmp_3 = packet.lookahead<p4calc_t>();
        tmp_4 = packet.lookahead<p4calc_t>();
        tmp_5 = packet.lookahead<p4calc_t>();
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

control MyIngress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    bit<48> tmp_6;
    bit<32> nbase;
    bit<64> ncount;
    bit<32> nselect;
    bit<32> ninput;
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
        nbase = hdr.p4calc.operand_b;
        ncount = 64w8589934592;
        ninput = hdr.p4calc.operand_a;
        hash<bit<32>, bit<32>, tuple<bit<32>>, bit<64>>(nselect, HashAlgorithm.crc32, nbase, { ninput }, ncount);
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
    apply {
        if (hdr.p4calc.isValid()) 
            calculate.apply();
        else 
            operation_drop_2();
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

