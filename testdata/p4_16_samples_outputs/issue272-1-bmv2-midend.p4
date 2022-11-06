#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

struct some_meta_t {
    bool flag;
}

struct H {
}

struct M {
    bool _some_meta_flag0;
}

control DeparserI(packet_out packet, in H hdr) {
    apply {
    }
}

parser ParserI(packet_in pk, out H hdr, inout M meta, inout standard_metadata_t std_meta) {
    state start {
        transition accept;
    }
}

control VerifyChecksumI(inout H hdr, inout M meta) {
    apply {
    }
}

control ComputeChecksumI(inout H hdr, inout M meta) {
    apply {
    }
}

control IngressI(inout H hdr, inout M meta, inout standard_metadata_t std_meta) {
    @hidden action issue2721bmv2l46() {
        meta._some_meta_flag0 = true;
    }
    @hidden table tbl_issue2721bmv2l46 {
        actions = {
            issue2721bmv2l46();
        }
        const default_action = issue2721bmv2l46();
    }
    apply {
        tbl_issue2721bmv2l46.apply();
    }
}

control EgressI(inout H hdr, inout M meta, inout standard_metadata_t std_meta) {
    apply {
    }
}

V1Switch<H, M>(ParserI(), VerifyChecksumI(), IngressI(), EgressI(), ComputeChecksumI(), DeparserI()) main;
