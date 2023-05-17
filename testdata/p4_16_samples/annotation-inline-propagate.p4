// Test program to verify that annotations are correctly propagated during
// ControlBlock and ParserBlock inlning.
//
// Expectation: all annotations should propagate when the block is inlined
// unless they are in an exclusion list.
//
// For this test, InnerParser (ParserBlock) and MyIngressInner (ControlBlock)
// should be inlined. All attached annotations except @name should be
// propagated to the caller of the inlined block.

#include <core.p4>
#include <v1model.p4>

struct metadata {
}

header hdr_t {
    bit<16> field;
}

struct headers {
    hdr_t   hdr;
}

@my_anno_1
@name("inner_parser")
parser InnerParser(
    packet_in buffer,
    out hdr_t a) {

    state start {
        buffer.extract(a);
        transition accept;
    }
}

@my_anno_2
@name("outer_parser")
parser MyParser(
    packet_in buffer,
    out headers hdr,
    inout metadata meta,
    inout standard_metadata_t standard_metadata) {

    InnerParser() inner;

    state start {
        inner.apply(buffer, hdr.hdr);
        transition accept;
    }
}

control MyVerifyChecksum(inout headers hdr, inout metadata meta) {
    apply {  }
}

@my_anno_3
@name("inner_ctrl")
control MyIngressInner(inout headers hdr) {
    action set_hdr(bit<16> val) {
        hdr.hdr.field = val;
    }

    @my_anno_4
    @name("inner_table")
    table table1 {
        actions = {
            set_hdr;
            NoAction;
        }
        default_action = NoAction;
    }

    apply {
        if (table1.apply().miss) {
            hdr.hdr.field = 0x1;
        }
    }
}

@my_anno_5
control MyIngress(inout headers hdr,
                  inout metadata meta,
                  inout standard_metadata_t standard_metadata) {
    MyIngressInner() inner;

    apply {
        inner.apply(hdr);
    }
}

control MyEgress(inout headers hdr,
                 inout metadata meta,
                 inout standard_metadata_t standard_metadata) {
    apply {  }
}

control MyComputeChecksum(inout headers hdr, inout metadata meta) {
    apply { }
}

control MyDeparser(packet_out packet, in headers hdr) {
    apply { }
}

V1Switch(
    MyParser(),
    MyVerifyChecksum(),
    MyIngress(),
    MyEgress(),
    MyComputeChecksum(),
    MyDeparser()
) main;
