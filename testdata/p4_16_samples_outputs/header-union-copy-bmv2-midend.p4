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
    @name("parsed_first") H parsed_first_0;
    @name("parsed_second") H parsed_second_0;
    state start {
        parsed_first_0.setInvalid();
        parsed_second_0.setInvalid();
        parsed_second_0.setInvalid();
        parsed_first_0.setInvalid();
        pkt.extract<H>(hdr.tag);
        transition select(hdr.tag.value) {
            8w0: first;
            8w1: second;
            default: copy;
        }
    }
    state first {
        pkt.extract<H>(parsed_first_0);
        parsed_second_0.setInvalid();
        transition copy;
    }
    state second {
        pkt.extract<H>(parsed_second_0);
        parsed_first_0.setInvalid();
        transition copy;
    }
    state copy {
        transition select((bit<1>)parsed_first_0.isValid()) {
            1w1: copy_true;
            1w0: copy_false;
        }
    }
    state copy_true {
        transition select(parsed_first_0.isValid()) {
            true: copy_true_true;
            false: copy_true_false;
        }
    }
    state copy_true_true {
        hdr.src_first.setValid();
        hdr.src_first = parsed_first_0;
        hdr.src_second.setInvalid();
        transition copy_true_join;
    }
    state copy_true_false {
        hdr.src_first.setInvalid();
        hdr.src_second.setInvalid();
        transition copy_true_join;
    }
    state copy_true_join {
        transition copy_join;
    }
    state copy_false {
        transition select((bit<1>)parsed_second_0.isValid()) {
            1w1: copy_false_true;
            1w0: copy_false_false;
        }
    }
    state copy_false_true {
        transition select(parsed_second_0.isValid()) {
            true: copy_false_true_true;
            false: copy_false_true_false;
        }
    }
    state copy_false_true_true {
        hdr.src_second.setValid();
        hdr.src_second = parsed_second_0;
        hdr.src_first.setInvalid();
        transition copy_false_true_join;
    }
    state copy_false_true_false {
        hdr.src_second.setInvalid();
        hdr.src_first.setInvalid();
        transition copy_false_true_join;
    }
    state copy_false_true_join {
        transition copy_false_join;
    }
    state copy_false_false {
        hdr.src_first.setInvalid();
        hdr.src_second.setInvalid();
        transition copy_false_join;
    }
    state copy_false_join {
        transition copy_join;
    }
    state copy_join {
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
    @name("tmp_0_first") H tmp_0_first_0;
    @name("tmp_0_second") H tmp_0_second_0;
    @hidden @name("headerunioncopybmv2l61") action headerunioncopybmv2l61_2() {
        if (hdr.src_first.isValid()) {
            hdr.dst_first.setValid();
            hdr.dst_first = hdr.src_first;
            hdr.dst_second.setInvalid();
        } else {
            hdr.dst_first.setInvalid();
            hdr.dst_second.setInvalid();
        }
    }
    @hidden @name("headerunioncopybmv2l61_0") action headerunioncopybmv2l61_3() {
        if (hdr.src_second.isValid()) {
            hdr.dst_second.setValid();
            hdr.dst_second = hdr.src_second;
            hdr.dst_first.setInvalid();
        } else {
            hdr.dst_second.setInvalid();
            hdr.dst_first.setInvalid();
        }
    }
    @hidden @name("headerunioncopybmv2l61_1") action headerunioncopybmv2l61_4() {
        hdr.dst_first.setInvalid();
        hdr.dst_second.setInvalid();
    }
    @hidden @name("headerunioncopybmv2l57") action headerunioncopybmv2l57_0() {
        hdr.dst_first.setValid();
        hdr.dst_second.setInvalid();
    }
    @hidden @name("headerunioncopybmv2l62") action headerunioncopybmv2l62_5() {
        if (hdr.dst_first.isValid()) {
            tmp_0_first_0.setValid();
            tmp_0_first_0 = hdr.dst_first;
            tmp_0_second_0.setInvalid();
        } else {
            tmp_0_first_0.setInvalid();
            tmp_0_second_0.setInvalid();
        }
    }
    @hidden @name("headerunioncopybmv2l62_0") action headerunioncopybmv2l62_6() {
        if (hdr.dst_second.isValid()) {
            tmp_0_second_0.setValid();
            tmp_0_second_0 = hdr.dst_second;
            tmp_0_first_0.setInvalid();
        } else {
            tmp_0_second_0.setInvalid();
            tmp_0_first_0.setInvalid();
        }
    }
    @hidden @name("headerunioncopybmv2l62_1") action headerunioncopybmv2l62_7() {
        tmp_0_first_0.setInvalid();
        tmp_0_second_0.setInvalid();
    }
    @hidden @name("headerunioncopybmv2l62_2") action headerunioncopybmv2l62_8() {
        if (tmp_0_first_0.isValid()) {
            hdr.dst_first.setValid();
            hdr.dst_first = tmp_0_first_0;
            hdr.dst_second.setInvalid();
        } else {
            hdr.dst_first.setInvalid();
            hdr.dst_second.setInvalid();
        }
    }
    @hidden @name("headerunioncopybmv2l62_3") action headerunioncopybmv2l62_9() {
        if (tmp_0_second_0.isValid()) {
            hdr.dst_second.setValid();
            hdr.dst_second = tmp_0_second_0;
            hdr.dst_first.setInvalid();
        } else {
            hdr.dst_second.setInvalid();
            hdr.dst_first.setInvalid();
        }
    }
    @hidden @name("headerunioncopybmv2l62_4") action headerunioncopybmv2l62_10() {
        hdr.dst_first.setInvalid();
        hdr.dst_second.setInvalid();
    }
    @hidden @name("headerunioncopybmv2l63") action headerunioncopybmv2l63_0() {
        sm.egress_spec = 9w1;
    }
    @hidden @name("tbl_headerunioncopybmv2l57") table tbl_headerunioncopybmv2l57_0 {
        actions = {
            headerunioncopybmv2l57_0();
        }
        const default_action = headerunioncopybmv2l57_0();
    }
    @hidden @name("tbl_headerunioncopybmv2l61") table tbl_headerunioncopybmv2l61_2 {
        actions = {
            headerunioncopybmv2l61_2();
        }
        const default_action = headerunioncopybmv2l61_2();
    }
    @hidden @name("tbl_headerunioncopybmv2l61_0") table tbl_headerunioncopybmv2l61_3 {
        actions = {
            headerunioncopybmv2l61_3();
        }
        const default_action = headerunioncopybmv2l61_3();
    }
    @hidden @name("tbl_headerunioncopybmv2l61_1") table tbl_headerunioncopybmv2l61_4 {
        actions = {
            headerunioncopybmv2l61_4();
        }
        const default_action = headerunioncopybmv2l61_4();
    }
    @hidden @name("tbl_headerunioncopybmv2l62") table tbl_headerunioncopybmv2l62_5 {
        actions = {
            headerunioncopybmv2l62_5();
        }
        const default_action = headerunioncopybmv2l62_5();
    }
    @hidden @name("tbl_headerunioncopybmv2l62_0") table tbl_headerunioncopybmv2l62_6 {
        actions = {
            headerunioncopybmv2l62_6();
        }
        const default_action = headerunioncopybmv2l62_6();
    }
    @hidden @name("tbl_headerunioncopybmv2l62_1") table tbl_headerunioncopybmv2l62_7 {
        actions = {
            headerunioncopybmv2l62_7();
        }
        const default_action = headerunioncopybmv2l62_7();
    }
    @hidden @name("tbl_headerunioncopybmv2l62_2") table tbl_headerunioncopybmv2l62_8 {
        actions = {
            headerunioncopybmv2l62_8();
        }
        const default_action = headerunioncopybmv2l62_8();
    }
    @hidden @name("tbl_headerunioncopybmv2l62_3") table tbl_headerunioncopybmv2l62_9 {
        actions = {
            headerunioncopybmv2l62_9();
        }
        const default_action = headerunioncopybmv2l62_9();
    }
    @hidden @name("tbl_headerunioncopybmv2l62_4") table tbl_headerunioncopybmv2l62_10 {
        actions = {
            headerunioncopybmv2l62_10();
        }
        const default_action = headerunioncopybmv2l62_10();
    }
    @hidden @name("tbl_headerunioncopybmv2l63") table tbl_headerunioncopybmv2l63_0 {
        actions = {
            headerunioncopybmv2l63_0();
        }
        const default_action = headerunioncopybmv2l63_0();
    }
    apply {
        tbl_headerunioncopybmv2l57_0.apply();
        if (hdr.src_first.isValid()) {
            tbl_headerunioncopybmv2l61_2.apply();
        } else if (hdr.src_second.isValid()) {
            tbl_headerunioncopybmv2l61_3.apply();
        } else {
            tbl_headerunioncopybmv2l61_4.apply();
        }
        if (hdr.dst_first.isValid()) {
            tbl_headerunioncopybmv2l62_5.apply();
        } else if (hdr.dst_second.isValid()) {
            tbl_headerunioncopybmv2l62_6.apply();
        } else {
            tbl_headerunioncopybmv2l62_7.apply();
        }
        if (tmp_0_first_0.isValid()) {
            tbl_headerunioncopybmv2l62_8.apply();
        } else if (tmp_0_second_0.isValid()) {
            tbl_headerunioncopybmv2l62_9.apply();
        } else {
            tbl_headerunioncopybmv2l62_10.apply();
        }
        tbl_headerunioncopybmv2l63_0.apply();
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
