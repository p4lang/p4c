#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header H {
    bit<8> value;
}

header_union U {
    H first;
    H second;
}

struct Headers {
    H    tag;
    U[2] src;
    U[2] dst;
}

struct Metadata {
}

parser Parse(packet_in pkt, out Headers hdr, inout Metadata meta, inout standard_metadata_t sm) {
    @name("Parse.parsed") U[2] parsed;
    state start {
        parsed[0].first.setInvalid();
        parsed[0].second.setInvalid();
        parsed[1].first.setInvalid();
        parsed[1].second.setInvalid();
        pkt.extract<H>(hdr.tag);
        transition select(hdr.tag.value) {
            8w0: first;
            8w1: second;
            default: copy;
        }
    }
    state first {
        pkt.extract<H>(parsed[0].first);
        transition copy;
    }
    state second {
        pkt.extract<H>(parsed[1].second);
        transition copy;
    }
    state copy {
        transition select((bit<1>)parsed[0].first.isValid()) {
            1w1: copy_true;
            1w0: copy_false;
        }
    }
    state copy_true {
        hdr.src[0].first = parsed[0].first;
        transition copy_join;
    }
    state copy_false {
        transition select((bit<1>)parsed[0].second.isValid()) {
            1w1: copy_false_true;
            1w0: copy_false_false;
        }
    }
    state copy_false_true {
        hdr.src[0].second = parsed[0].second;
        transition copy_false_join;
    }
    state copy_false_false {
        hdr.src[0].first.setInvalid();
        transition copy_false_join;
    }
    state copy_false_join {
        transition copy_join;
    }
    state copy_join {
        transition select((bit<1>)parsed[1].first.isValid()) {
            1w1: copy_true_0;
            1w0: copy_false_0;
        }
    }
    state copy_true_0 {
        hdr.src[1].first = parsed[1].first;
        transition copy_join_0;
    }
    state copy_false_0 {
        transition select((bit<1>)parsed[1].second.isValid()) {
            1w1: copy_false_0_true;
            1w0: copy_false_0_false;
        }
    }
    state copy_false_0_true {
        hdr.src[1].second = parsed[1].second;
        transition copy_false_0_join;
    }
    state copy_false_0_false {
        hdr.src[1].first.setInvalid();
        transition copy_false_0_join;
    }
    state copy_false_0_join {
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
    bit<1> hsiVar;
    H tmp_0_first;
    H tmp_0_second;
    @name("TestIngress.copies") U[2] copies;
    @hidden action headerunionstackcopybmv2l47() {
        copies[0].first = hdr.src[0].first;
    }
    @hidden action headerunionstackcopybmv2l47_0() {
        copies[0].second = hdr.src[0].second;
    }
    @hidden action headerunionstackcopybmv2l47_1() {
        copies[0].first.setInvalid();
    }
    @hidden action headerunionstackcopybmv2l45() {
        copies[0].first.setInvalid();
        copies[0].second.setInvalid();
        copies[1].first.setInvalid();
        copies[1].second.setInvalid();
    }
    @hidden action headerunionstackcopybmv2l48() {
        copies[1].first = hdr.src[1].first;
    }
    @hidden action headerunionstackcopybmv2l48_0() {
        copies[1].second = hdr.src[1].second;
    }
    @hidden action headerunionstackcopybmv2l48_1() {
        copies[1].first.setInvalid();
    }
    @hidden action headerunionstackcopybmv2l50() {
        if (copies[1w0].first.isValid()) {
            tmp_0_first.setValid();
            tmp_0_first = copies[1w0].first;
            tmp_0_second.setInvalid();
        } else {
            tmp_0_first.setInvalid();
            tmp_0_second.setInvalid();
        }
    }
    @hidden action headerunionstackcopybmv2l50_0() {
        if (copies[1w1].first.isValid()) {
            tmp_0_first.setValid();
            tmp_0_first = copies[1w1].first;
            tmp_0_second.setInvalid();
        } else {
            tmp_0_first.setInvalid();
            tmp_0_second.setInvalid();
        }
    }
    @hidden action headerunionstackcopybmv2l50_1() {
        if (copies[1w0].second.isValid()) {
            tmp_0_second.setValid();
            tmp_0_second = copies[1w0].second;
            tmp_0_first.setInvalid();
        } else {
            tmp_0_second.setInvalid();
            tmp_0_first.setInvalid();
        }
    }
    @hidden action headerunionstackcopybmv2l50_2() {
        if (copies[1w1].second.isValid()) {
            tmp_0_second.setValid();
            tmp_0_second = copies[1w1].second;
            tmp_0_first.setInvalid();
        } else {
            tmp_0_second.setInvalid();
            tmp_0_first.setInvalid();
        }
    }
    @hidden action headerunionstackcopybmv2l50_3() {
        tmp_0_first.setInvalid();
        tmp_0_second.setInvalid();
    }
    @hidden action headerunionstackcopybmv2l50_4() {
        hsiVar = hdr.tag.value[0:0];
    }
    @hidden action headerunionstackcopybmv2l50_5() {
        hsiVar = hdr.tag.value[0:0];
    }
    @hidden action headerunionstackcopybmv2l50_6() {
        hdr.dst[1w0].first = tmp_0_first;
    }
    @hidden action headerunionstackcopybmv2l50_7() {
        hdr.dst[1w1].first = tmp_0_first;
    }
    @hidden action headerunionstackcopybmv2l50_8() {
        hsiVar = hdr.tag.value[0:0];
    }
    @hidden action headerunionstackcopybmv2l50_9() {
        hdr.dst[1w0].second = tmp_0_second;
    }
    @hidden action headerunionstackcopybmv2l50_10() {
        hdr.dst[1w1].second = tmp_0_second;
    }
    @hidden action headerunionstackcopybmv2l50_11() {
        hsiVar = hdr.tag.value[0:0];
    }
    @hidden action headerunionstackcopybmv2l50_12() {
        hdr.dst[1w0].first.setInvalid();
    }
    @hidden action headerunionstackcopybmv2l50_13() {
        hdr.dst[1w1].first.setInvalid();
    }
    @hidden action headerunionstackcopybmv2l50_14() {
        hsiVar = hdr.tag.value[0:0];
    }
    @hidden action headerunionstackcopybmv2l51() {
        sm.egress_spec = 9w1;
    }
    @hidden table tbl_headerunionstackcopybmv2l45 {
        actions = {
            headerunionstackcopybmv2l45();
        }
        const default_action = headerunionstackcopybmv2l45();
    }
    @hidden table tbl_headerunionstackcopybmv2l47 {
        actions = {
            headerunionstackcopybmv2l47();
        }
        const default_action = headerunionstackcopybmv2l47();
    }
    @hidden table tbl_headerunionstackcopybmv2l47_0 {
        actions = {
            headerunionstackcopybmv2l47_0();
        }
        const default_action = headerunionstackcopybmv2l47_0();
    }
    @hidden table tbl_headerunionstackcopybmv2l47_1 {
        actions = {
            headerunionstackcopybmv2l47_1();
        }
        const default_action = headerunionstackcopybmv2l47_1();
    }
    @hidden table tbl_headerunionstackcopybmv2l48 {
        actions = {
            headerunionstackcopybmv2l48();
        }
        const default_action = headerunionstackcopybmv2l48();
    }
    @hidden table tbl_headerunionstackcopybmv2l48_0 {
        actions = {
            headerunionstackcopybmv2l48_0();
        }
        const default_action = headerunionstackcopybmv2l48_0();
    }
    @hidden table tbl_headerunionstackcopybmv2l48_1 {
        actions = {
            headerunionstackcopybmv2l48_1();
        }
        const default_action = headerunionstackcopybmv2l48_1();
    }
    @hidden table tbl_headerunionstackcopybmv2l50 {
        actions = {
            headerunionstackcopybmv2l50_5();
        }
        const default_action = headerunionstackcopybmv2l50_5();
    }
    @hidden table tbl_headerunionstackcopybmv2l50_0 {
        actions = {
            headerunionstackcopybmv2l50();
        }
        const default_action = headerunionstackcopybmv2l50();
    }
    @hidden table tbl_headerunionstackcopybmv2l50_1 {
        actions = {
            headerunionstackcopybmv2l50_0();
        }
        const default_action = headerunionstackcopybmv2l50_0();
    }
    @hidden table tbl_headerunionstackcopybmv2l50_2 {
        actions = {
            headerunionstackcopybmv2l50_4();
        }
        const default_action = headerunionstackcopybmv2l50_4();
    }
    @hidden table tbl_headerunionstackcopybmv2l50_3 {
        actions = {
            headerunionstackcopybmv2l50_1();
        }
        const default_action = headerunionstackcopybmv2l50_1();
    }
    @hidden table tbl_headerunionstackcopybmv2l50_4 {
        actions = {
            headerunionstackcopybmv2l50_2();
        }
        const default_action = headerunionstackcopybmv2l50_2();
    }
    @hidden table tbl_headerunionstackcopybmv2l50_5 {
        actions = {
            headerunionstackcopybmv2l50_3();
        }
        const default_action = headerunionstackcopybmv2l50_3();
    }
    @hidden table tbl_headerunionstackcopybmv2l50_6 {
        actions = {
            headerunionstackcopybmv2l50_8();
        }
        const default_action = headerunionstackcopybmv2l50_8();
    }
    @hidden table tbl_headerunionstackcopybmv2l50_7 {
        actions = {
            headerunionstackcopybmv2l50_6();
        }
        const default_action = headerunionstackcopybmv2l50_6();
    }
    @hidden table tbl_headerunionstackcopybmv2l50_8 {
        actions = {
            headerunionstackcopybmv2l50_7();
        }
        const default_action = headerunionstackcopybmv2l50_7();
    }
    @hidden table tbl_headerunionstackcopybmv2l50_9 {
        actions = {
            headerunionstackcopybmv2l50_11();
        }
        const default_action = headerunionstackcopybmv2l50_11();
    }
    @hidden table tbl_headerunionstackcopybmv2l50_10 {
        actions = {
            headerunionstackcopybmv2l50_9();
        }
        const default_action = headerunionstackcopybmv2l50_9();
    }
    @hidden table tbl_headerunionstackcopybmv2l50_11 {
        actions = {
            headerunionstackcopybmv2l50_10();
        }
        const default_action = headerunionstackcopybmv2l50_10();
    }
    @hidden table tbl_headerunionstackcopybmv2l50_12 {
        actions = {
            headerunionstackcopybmv2l50_14();
        }
        const default_action = headerunionstackcopybmv2l50_14();
    }
    @hidden table tbl_headerunionstackcopybmv2l50_13 {
        actions = {
            headerunionstackcopybmv2l50_12();
        }
        const default_action = headerunionstackcopybmv2l50_12();
    }
    @hidden table tbl_headerunionstackcopybmv2l50_14 {
        actions = {
            headerunionstackcopybmv2l50_13();
        }
        const default_action = headerunionstackcopybmv2l50_13();
    }
    @hidden table tbl_headerunionstackcopybmv2l51 {
        actions = {
            headerunionstackcopybmv2l51();
        }
        const default_action = headerunionstackcopybmv2l51();
    }
    apply {
        tbl_headerunionstackcopybmv2l45.apply();
        if (hdr.src[0].first.isValid()) {
            tbl_headerunionstackcopybmv2l47.apply();
        } else if (hdr.src[0].second.isValid()) {
            tbl_headerunionstackcopybmv2l47_0.apply();
        } else {
            tbl_headerunionstackcopybmv2l47_1.apply();
        }
        if (hdr.src[1].first.isValid()) {
            tbl_headerunionstackcopybmv2l48.apply();
        } else if (hdr.src[1].second.isValid()) {
            tbl_headerunionstackcopybmv2l48_0.apply();
        } else {
            tbl_headerunionstackcopybmv2l48_1.apply();
        }
        tbl_headerunionstackcopybmv2l50.apply();
        if (hsiVar == 1w0 && copies[1w0].first.isValid()) {
            tbl_headerunionstackcopybmv2l50_0.apply();
        } else if (hsiVar == 1w1 && copies[1w1].first.isValid()) {
            tbl_headerunionstackcopybmv2l50_1.apply();
        } else {
            tbl_headerunionstackcopybmv2l50_2.apply();
            if (hsiVar == 1w0 && copies[1w0].second.isValid()) {
                tbl_headerunionstackcopybmv2l50_3.apply();
            } else if (hsiVar == 1w1 && copies[1w1].second.isValid()) {
                tbl_headerunionstackcopybmv2l50_4.apply();
            } else {
                tbl_headerunionstackcopybmv2l50_5.apply();
            }
        }
        if (tmp_0_first.isValid()) {
            tbl_headerunionstackcopybmv2l50_6.apply();
            if (hsiVar == 1w0) {
                tbl_headerunionstackcopybmv2l50_7.apply();
            } else if (hsiVar == 1w1) {
                tbl_headerunionstackcopybmv2l50_8.apply();
            }
        } else if (tmp_0_second.isValid()) {
            tbl_headerunionstackcopybmv2l50_9.apply();
            if (hsiVar == 1w0) {
                tbl_headerunionstackcopybmv2l50_10.apply();
            } else if (hsiVar == 1w1) {
                tbl_headerunionstackcopybmv2l50_11.apply();
            }
        } else {
            tbl_headerunionstackcopybmv2l50_12.apply();
            if (hsiVar == 1w0) {
                tbl_headerunionstackcopybmv2l50_13.apply();
            } else if (hsiVar == 1w1) {
                tbl_headerunionstackcopybmv2l50_14.apply();
            }
        }
        tbl_headerunionstackcopybmv2l51.apply();
    }
}

control Deparse(packet_out pkt, in Headers hdr) {
    apply {
        pkt.emit<H>(hdr.tag);
        pkt.emit<H>(hdr.dst[0].first);
        pkt.emit<H>(hdr.dst[0].second);
        pkt.emit<H>(hdr.dst[1].first);
        pkt.emit<H>(hdr.dst[1].second);
    }
}

V1Switch<Headers, Metadata>(Parse(), Verify(), TestIngress(), TestEgress(), Update(), Deparse()) main;
