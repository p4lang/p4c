#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header H {
    bit<8> value;
}

struct Headers {
    H tag;
    H src_first;
    H src_second;
    H dst_first;
    H dst_second;
}

struct Metadata {
}

parser Parse(packet_in pkt, out Headers hdr, inout Metadata meta, inout standard_metadata_t sm) {
    @name("parsed_0_first") H parsed_0_first_0;
    @name("parsed_0_second") H parsed_0_second_0;
    state start {
        parsed_0_first_0.setInvalid();
        parsed_0_second_0.setInvalid();
        pkt.extract<H>(hdr.tag);
        transition select(hdr.tag.value) {
            8w0: first;
            8w1: second;
            default: copy;
        }
    }
    state first {
        pkt.extract<H>(parsed_0_first_0);
        parsed_0_second_0.setInvalid();
        transition copy;
    }
    state second {
        pkt.extract<H>(parsed_0_second_0);
        parsed_0_first_0.setInvalid();
        transition copy;
    }
    state copy {
        transition select(parsed_0_first_0.isValid()) {
            true: copy_true;
            false: copy_false;
        }
    }
    state copy_true {
        hdr.src_first.setValid();
        hdr.src_first = parsed_0_first_0;
        hdr.src_second.setInvalid();
        transition copy_join;
    }
    state copy_false {
        hdr.src_first.setInvalid();
        transition copy_join;
    }
    state copy_join {
        transition select(parsed_0_second_0.isValid()) {
            true: copy_true_0;
            false: copy_false_0;
        }
    }
    state copy_true_0 {
        hdr.src_second.setValid();
        hdr.src_second = parsed_0_second_0;
        hdr.src_first.setInvalid();
        transition copy_join_0;
    }
    state copy_false_0 {
        hdr.src_second.setInvalid();
        transition copy_join_0;
    }
    state copy_join_0 {
        transition accept;
    }
}

control Verify(inout Headers hdr, inout Metadata meta) {
    apply {
    }
}

control Update(inout Headers hdr, inout Metadata meta) {
    apply {
    }
}

control TestEgress(inout Headers hdr, inout Metadata meta, inout standard_metadata_t sm) {
    apply {
    }
}

control TestIngress(inout Headers hdr, inout Metadata meta, inout standard_metadata_t sm) {
    @hidden @name("headerunioncopybmv2l57") action headerunioncopybmv2l57_0() {
        hdr.dst_first.setValid();
        hdr.dst_second.setInvalid();
        if (hdr.src_first.isValid()) {
            hdr.dst_first.setValid();
            hdr.dst_first = hdr.src_first;
            hdr.dst_second.setInvalid();
        } else {
            hdr.dst_first.setInvalid();
        }
        if (hdr.src_second.isValid()) {
            hdr.dst_second.setValid();
            hdr.dst_second = hdr.src_second;
            hdr.dst_first.setInvalid();
        } else {
            hdr.dst_second.setInvalid();
        }
        if (hdr.src_first.isValid()) {
            hdr.dst_first.setValid();
            hdr.dst_first = hdr.src_first;
            hdr.dst_second.setInvalid();
        } else {
            hdr.dst_first.setInvalid();
        }
        if (hdr.src_second.isValid()) {
            hdr.dst_second.setValid();
            hdr.dst_second = hdr.src_second;
            hdr.dst_first.setInvalid();
        } else {
            hdr.dst_second.setInvalid();
        }
        sm.egress_spec = 9w1;
    }
    @hidden @name("tbl_headerunioncopybmv2l57") table tbl_headerunioncopybmv2l57_0 {
        actions = {
            headerunioncopybmv2l57_0();
        }
        const default_action = headerunioncopybmv2l57_0();
    }
    apply {
        tbl_headerunioncopybmv2l57_0.apply();
    }
}

control Deparse(packet_out pkt, in Headers hdr) {
    apply {
        pkt.emit<H>(hdr.tag);
        pkt.emit<H>(hdr.dst_first);
        pkt.emit<H>(hdr.dst_second);
    }
}

V1Switch<Headers, Metadata>(Parse(), Verify(), TestIngress(), TestEgress(), Update(), Deparse()) main;
