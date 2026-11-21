#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

struct metadata {
}

header hdr_t {
    bit<16> field;
}

struct headers {
    hdr_t hdr;
}

@my_anno_1 @name("inner_parser") parser InnerParser(packet_in buffer, out hdr_t a) {
    state start {
        buffer.extract<hdr_t>(a);
        transition accept;
    }
}

@my_anno_2 @name("outer_parser") parser MyParser(packet_in buffer, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    InnerParser() inner;
    state start {
        inner.apply(buffer, hdr.hdr);
        transition accept;
    }
}

control MyVerifyChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

@my_anno_3 @name("inner_ctrl") control MyIngressInner(inout headers hdr) {
    action set_hdr(bit<16> val) {
        hdr.hdr.field = val;
    }
    @my_anno_4 @name("inner_table") table table1 {
        actions = {
            set_hdr();
            NoAction();
        }
        default_action = NoAction();
    }
    apply {
        if (table1.apply().miss) {
            hdr.hdr.field = 16w0x1;
        }
    }
}

@my_anno_5 control MyIngress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    MyIngressInner() inner;
    apply {
        inner.apply(hdr);
    }
}

control MyEgress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control MyComputeChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

control MyDeparser(packet_out packet, in headers hdr) {
    apply {
    }
}

V1Switch<headers, metadata>(MyParser(), MyVerifyChecksum(), MyIngress(), MyEgress(), MyComputeChecksum(), MyDeparser()) main;
