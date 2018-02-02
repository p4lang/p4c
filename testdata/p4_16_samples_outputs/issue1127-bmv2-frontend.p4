#include <core.p4>
#include <v1model.p4>

header h1_t {
    bit<8> op1;
    bit<8> op2;
    bit<8> out1;
}

struct headers {
    h1_t h1;
}

struct metadata {
}

parser parserI(packet_in pkt, out headers hdr, inout metadata meta, inout standard_metadata_t stdmeta) {
    state start {
        pkt.extract<h1_t>(hdr.h1);
        transition accept;
    }
}

control cIngress(inout headers hdr, inout metadata meta, inout standard_metadata_t stdmeta) {
    headers tmp_3;
    bit<8> tmp_4;
    headers tmp_5;
    bit<8> tmp_6;
    headers hdr_1;
    bit<8> op;
    apply {
        tmp_3 = hdr;
        tmp_4 = hdr.h1.op1;
        hdr_1 = tmp_3;
        op = tmp_4;
        if (op == 8w0x0) 
            ;
        else 
            if (op[7:4] == 4w1) 
                hdr_1.h1.out1 = 8w4;
        tmp_3 = hdr_1;
        hdr = tmp_3;
        tmp_5 = hdr;
        tmp_6 = hdr.h1.op2;
        hdr_1 = tmp_5;
        op = tmp_6;
        if (op == 8w0x0) 
            ;
        else 
            if (op[7:4] == 4w1) 
                hdr_1.h1.out1 = 8w4;
        tmp_5 = hdr_1;
        hdr = tmp_5;
    }
}

control cEgress(inout headers hdr, inout metadata meta, inout standard_metadata_t stdmeta) {
    apply {
    }
}

control vc(inout headers hdr, inout metadata meta) {
    apply {
    }
}

control uc(inout headers hdr, inout metadata meta) {
    apply {
    }
}

control DeparserI(packet_out packet, in headers hdr) {
    apply {
        packet.emit<h1_t>(hdr.h1);
    }
}

V1Switch<headers, metadata>(parserI(), vc(), cIngress(), cEgress(), uc(), DeparserI()) main;

