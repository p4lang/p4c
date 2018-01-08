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

control cDoOneOp(inout headers hdr, in bit<8> op) {
    apply {
        if (op == 8w0x0) 
            ;
        else 
            if (op[7:4] == 4w1) 
                hdr.h1.out1 = 8w4;
    }
}

control cIngress(inout headers hdr, inout metadata meta, inout standard_metadata_t stdmeta) {
    headers tmp;
    bit<8> tmp_0;
    headers tmp_1;
    bit<8> tmp_2;
    @name("do_one_op") cDoOneOp() do_one_op_0;
    apply {
        tmp = hdr;
        tmp_0 = hdr.h1.op1;
        do_one_op_0.apply(tmp, tmp_0);
        hdr = tmp;
        tmp_1 = hdr;
        tmp_2 = hdr.h1.op2;
        do_one_op_0.apply(tmp_1, tmp_2);
        hdr = tmp_1;
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

