#include <core.p4>
#include <v1model.p4>

@name("hdr") header hdr_0 {
    bit<8>  op;
    @saturating 
    bit<8>  opr1_8;
    @saturating 
    bit<8>  opr2_8;
    @saturating 
    bit<8>  res_8;
    @saturating 
    int<16> opr1_16;
    @saturating 
    int<16> opr2_16;
    @saturating 
    int<16> res_16;
}

struct metadata {
}

struct headers {
    @name(".data") 
    hdr_0 data;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".start") state start {
        packet.extract<hdr_0>(hdr.data);
        transition accept;
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".sat_plus") action sat_plus() {
        standard_metadata.egress_spec = 9w0;
        hdr.data.res_8 = hdr.data.opr1_8 |+| hdr.data.opr2_8;
    }
    @name(".sat_minus") action sat_minus() {
        standard_metadata.egress_spec = 9w0;
        hdr.data.res_8 = hdr.data.opr1_8 |-| hdr.data.opr2_8;
    }
    @name(".sat_add_to") action sat_add_to() {
        standard_metadata.egress_spec = 9w0;
        hdr.data.res_16 = hdr.data.opr1_16;
        hdr.data.res_16 = hdr.data.opr1_16 |+| hdr.data.opr2_16;
    }
    @name(".sat_subtract_from") action sat_subtract_from() {
        standard_metadata.egress_spec = 9w0;
        hdr.data.res_16 = hdr.data.opr1_16;
        hdr.data.res_16 = hdr.data.opr1_16 |-| hdr.data.opr2_16;
    }
    @name("._drop") action _drop() {
        mark_to_drop();
    }
    @name(".t") table t_0 {
        actions = {
            sat_plus();
            sat_minus();
            sat_add_to();
            sat_subtract_from();
            _drop();
        }
        key = {
            hdr.data.op: exact @name("data.op") ;
        }
        default_action = _drop();
    }
    apply {
        t_0.apply();
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit<hdr_0>(hdr.data);
    }
}

control verifyChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

V1Switch<headers, metadata>(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;

