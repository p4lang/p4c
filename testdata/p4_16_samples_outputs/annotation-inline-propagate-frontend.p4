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

@my_anno_2 @name("MyParser.outer_parser") @my_anno_1 parser MyParser(packet_in buffer, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    state start {
        hdr.hdr.setInvalid();
        transition InnerParser_start;
    }
    state InnerParser_start {
        buffer.extract<hdr_t>(hdr.hdr);
        transition start_0;
    }
    state start_0 {
        transition accept;
    }
}

control MyVerifyChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

@my_anno_5 @my_anno_3 control MyIngress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("MyIngress.inner.set_hdr") action inner_set_hdr_0(@name("val") bit<16> val) {
        hdr.hdr.field = val;
    }
    @my_anno_4 @name("MyIngress.inner.inner_table") table inner_inner_table {
        actions = {
            inner_set_hdr_0();
            NoAction_1();
        }
        default_action = NoAction_1();
    }
    apply {
        @my_anno_3 {
            if (inner_inner_table.apply().miss) {
                hdr.hdr.field = 16w0x1;
            }
        }
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
