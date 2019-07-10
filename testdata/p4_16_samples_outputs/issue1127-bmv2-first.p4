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
        if (op == 8w0x0) {
            ;
        } else if (op[7:4] == 4w1) {
            hdr.h1.out1 = 8w4;
        }
    }
}

control cIngress(inout headers hdr, inout metadata meta, inout standard_metadata_t stdmeta) {
    cDoOneOp() do_one_op;
    apply {
        do_one_op.apply(hdr, hdr.h1.op1);
        do_one_op.apply(hdr, hdr.h1.op2);
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

