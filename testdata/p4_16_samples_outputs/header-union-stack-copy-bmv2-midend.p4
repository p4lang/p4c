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
        transition select(parsed[0].first.isValid()) {
            true: copy_true;
            false: copy_false;
        }
    }
    state copy_true {
        hdr.src[0].first.setValid();
        hdr.src[0].first = parsed[0].first;
        hdr.src[0].second.setInvalid();
        transition copy_join;
    }
    state copy_false {
        hdr.src[0].first.setInvalid();
        transition copy_join;
    }
    state copy_join {
        transition select(parsed[0].second.isValid()) {
            true: copy_true_0;
            false: copy_false_0;
        }
    }
    state copy_true_0 {
        hdr.src[0].second.setValid();
        hdr.src[0].second = parsed[0].second;
        hdr.src[0].first.setInvalid();
        transition copy_join_0;
    }
    state copy_false_0 {
        hdr.src[0].second.setInvalid();
        transition copy_join_0;
    }
    state copy_join_0 {
        transition select(parsed[1].first.isValid()) {
            true: copy_true_1;
            false: copy_false_1;
        }
    }
    state copy_true_1 {
        hdr.src[1].first.setValid();
        hdr.src[1].first = parsed[1].first;
        hdr.src[1].second.setInvalid();
        transition copy_join_1;
    }
    state copy_false_1 {
        hdr.src[1].first.setInvalid();
        transition copy_join_1;
    }
    state copy_join_1 {
        transition select(parsed[1].second.isValid()) {
            true: copy_true_2;
            false: copy_false_2;
        }
    }
    state copy_true_2 {
        hdr.src[1].second.setValid();
        hdr.src[1].second = parsed[1].second;
        hdr.src[1].first.setInvalid();
        transition copy_join_2;
    }
    state copy_false_2 {
        hdr.src[1].second.setInvalid();
        transition copy_join_2;
    }
    state copy_join_2 {
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
    @name("hsiVar") bit<1> hsiVar_0;
    @name("hsVar") H hsVar_0;
    @name("TestIngress.copies") U[2] copies;
    @hidden @name("headerunionstackcopybmv2l50") action headerunionstackcopybmv2l50_16() {
        if (copies[1w0].first.isValid()) {
            hdr.dst[1w0].first.setValid();
            hdr.dst[1w0].first = copies[1w0].first;
            hdr.dst[1w0].second.setInvalid();
        } else {
            hdr.dst[1w0].first.setInvalid();
        }
    }
    @hidden @name("headerunionstackcopybmv2l50_0") action headerunionstackcopybmv2l50_17() {
        if (copies[1w1].first.isValid()) {
            hdr.dst[1w0].first.setValid();
            hdr.dst[1w0].first = copies[1w1].first;
            hdr.dst[1w0].second.setInvalid();
        } else {
            hdr.dst[1w0].first.setInvalid();
        }
    }
    @hidden @name("headerunionstackcopybmv2l50_1") action headerunionstackcopybmv2l50_18() {
        if (hsVar_0.isValid()) {
            hdr.dst[1w0].first.setValid();
            hdr.dst[1w0].first = hsVar_0;
            hdr.dst[1w0].second.setInvalid();
        } else {
            hdr.dst[1w0].first.setInvalid();
        }
    }
    @hidden @name("headerunionstackcopybmv2l50_2") action headerunionstackcopybmv2l50_19() {
        hsiVar_0 = hdr.tag.value[0:0];
    }
    @hidden @name("headerunionstackcopybmv2l50_3") action headerunionstackcopybmv2l50_20() {
        if (copies[1w0].first.isValid()) {
            hdr.dst[1w1].first.setValid();
            hdr.dst[1w1].first = copies[1w0].first;
            hdr.dst[1w1].second.setInvalid();
        } else {
            hdr.dst[1w1].first.setInvalid();
        }
    }
    @hidden @name("headerunionstackcopybmv2l50_4") action headerunionstackcopybmv2l50_21() {
        if (copies[1w1].first.isValid()) {
            hdr.dst[1w1].first.setValid();
            hdr.dst[1w1].first = copies[1w1].first;
            hdr.dst[1w1].second.setInvalid();
        } else {
            hdr.dst[1w1].first.setInvalid();
        }
    }
    @hidden @name("headerunionstackcopybmv2l50_5") action headerunionstackcopybmv2l50_22() {
        if (hsVar_0.isValid()) {
            hdr.dst[1w1].first.setValid();
            hdr.dst[1w1].first = hsVar_0;
            hdr.dst[1w1].second.setInvalid();
        } else {
            hdr.dst[1w1].first.setInvalid();
        }
    }
    @hidden @name("headerunionstackcopybmv2l50_6") action headerunionstackcopybmv2l50_23() {
        hsiVar_0 = hdr.tag.value[0:0];
    }
    @hidden @name("headerunionstackcopybmv2l45") action headerunionstackcopybmv2l45_0() {
        copies[0].first.setInvalid();
        copies[0].second.setInvalid();
        copies[1].first.setInvalid();
        copies[1].second.setInvalid();
        if (hdr.src[0].first.isValid()) {
            copies[0].first.setValid();
            copies[0].first = hdr.src[0].first;
            copies[0].second.setInvalid();
        } else {
            copies[0].first.setInvalid();
        }
        if (hdr.src[0].second.isValid()) {
            copies[0].second.setValid();
            copies[0].second = hdr.src[0].second;
            copies[0].first.setInvalid();
        } else {
            copies[0].second.setInvalid();
        }
        if (hdr.src[1].first.isValid()) {
            copies[1].first.setValid();
            copies[1].first = hdr.src[1].first;
            copies[1].second.setInvalid();
        } else {
            copies[1].first.setInvalid();
        }
        if (hdr.src[1].second.isValid()) {
            copies[1].second.setValid();
            copies[1].second = hdr.src[1].second;
            copies[1].first.setInvalid();
        } else {
            copies[1].second.setInvalid();
        }
        hsiVar_0 = hdr.tag.value[0:0];
    }
    @hidden @name("headerunionstackcopybmv2l50_7") action headerunionstackcopybmv2l50_24() {
        if (copies[1w0].second.isValid()) {
            hdr.dst[1w0].second.setValid();
            hdr.dst[1w0].second = copies[1w0].second;
            hdr.dst[1w0].first.setInvalid();
        } else {
            hdr.dst[1w0].second.setInvalid();
        }
    }
    @hidden @name("headerunionstackcopybmv2l50_8") action headerunionstackcopybmv2l50_25() {
        if (copies[1w1].second.isValid()) {
            hdr.dst[1w0].second.setValid();
            hdr.dst[1w0].second = copies[1w1].second;
            hdr.dst[1w0].first.setInvalid();
        } else {
            hdr.dst[1w0].second.setInvalid();
        }
    }
    @hidden @name("headerunionstackcopybmv2l50_9") action headerunionstackcopybmv2l50_26() {
        if (hsVar_0.isValid()) {
            hdr.dst[1w0].second.setValid();
            hdr.dst[1w0].second = hsVar_0;
            hdr.dst[1w0].first.setInvalid();
        } else {
            hdr.dst[1w0].second.setInvalid();
        }
    }
    @hidden @name("headerunionstackcopybmv2l50_10") action headerunionstackcopybmv2l50_27() {
        hsiVar_0 = hdr.tag.value[0:0];
    }
    @hidden @name("headerunionstackcopybmv2l50_11") action headerunionstackcopybmv2l50_28() {
        if (copies[1w0].second.isValid()) {
            hdr.dst[1w1].second.setValid();
            hdr.dst[1w1].second = copies[1w0].second;
            hdr.dst[1w1].first.setInvalid();
        } else {
            hdr.dst[1w1].second.setInvalid();
        }
    }
    @hidden @name("headerunionstackcopybmv2l50_12") action headerunionstackcopybmv2l50_29() {
        if (copies[1w1].second.isValid()) {
            hdr.dst[1w1].second.setValid();
            hdr.dst[1w1].second = copies[1w1].second;
            hdr.dst[1w1].first.setInvalid();
        } else {
            hdr.dst[1w1].second.setInvalid();
        }
    }
    @hidden @name("headerunionstackcopybmv2l50_13") action headerunionstackcopybmv2l50_30() {
        if (hsVar_0.isValid()) {
            hdr.dst[1w1].second.setValid();
            hdr.dst[1w1].second = hsVar_0;
            hdr.dst[1w1].first.setInvalid();
        } else {
            hdr.dst[1w1].second.setInvalid();
        }
    }
    @hidden @name("headerunionstackcopybmv2l50_14") action headerunionstackcopybmv2l50_31() {
        hsiVar_0 = hdr.tag.value[0:0];
    }
    @hidden @name("headerunionstackcopybmv2l50_15") action headerunionstackcopybmv2l50_32() {
        hsiVar_0 = hdr.tag.value[0:0];
    }
    @hidden @name("headerunionstackcopybmv2l51") action headerunionstackcopybmv2l51_0() {
        sm.egress_spec = 9w1;
    }
    @hidden @name("tbl_headerunionstackcopybmv2l45") table tbl_headerunionstackcopybmv2l45_0 {
        actions = {
            headerunionstackcopybmv2l45_0();
        }
        const default_action = headerunionstackcopybmv2l45_0();
    }
    @hidden @name("tbl_headerunionstackcopybmv2l50") table tbl_headerunionstackcopybmv2l50_16 {
        actions = {
            headerunionstackcopybmv2l50_19();
        }
        const default_action = headerunionstackcopybmv2l50_19();
    }
    @hidden @name("tbl_headerunionstackcopybmv2l50_0") table tbl_headerunionstackcopybmv2l50_17 {
        actions = {
            headerunionstackcopybmv2l50_16();
        }
        const default_action = headerunionstackcopybmv2l50_16();
    }
    @hidden @name("tbl_headerunionstackcopybmv2l50_1") table tbl_headerunionstackcopybmv2l50_18 {
        actions = {
            headerunionstackcopybmv2l50_17();
        }
        const default_action = headerunionstackcopybmv2l50_17();
    }
    @hidden @name("tbl_headerunionstackcopybmv2l50_2") table tbl_headerunionstackcopybmv2l50_19 {
        actions = {
            headerunionstackcopybmv2l50_18();
        }
        const default_action = headerunionstackcopybmv2l50_18();
    }
    @hidden @name("tbl_headerunionstackcopybmv2l50_3") table tbl_headerunionstackcopybmv2l50_20 {
        actions = {
            headerunionstackcopybmv2l50_23();
        }
        const default_action = headerunionstackcopybmv2l50_23();
    }
    @hidden @name("tbl_headerunionstackcopybmv2l50_4") table tbl_headerunionstackcopybmv2l50_21 {
        actions = {
            headerunionstackcopybmv2l50_20();
        }
        const default_action = headerunionstackcopybmv2l50_20();
    }
    @hidden @name("tbl_headerunionstackcopybmv2l50_5") table tbl_headerunionstackcopybmv2l50_22 {
        actions = {
            headerunionstackcopybmv2l50_21();
        }
        const default_action = headerunionstackcopybmv2l50_21();
    }
    @hidden @name("tbl_headerunionstackcopybmv2l50_6") table tbl_headerunionstackcopybmv2l50_23 {
        actions = {
            headerunionstackcopybmv2l50_22();
        }
        const default_action = headerunionstackcopybmv2l50_22();
    }
    @hidden @name("tbl_headerunionstackcopybmv2l50_7") table tbl_headerunionstackcopybmv2l50_24 {
        actions = {
            headerunionstackcopybmv2l50_32();
        }
        const default_action = headerunionstackcopybmv2l50_32();
    }
    @hidden @name("tbl_headerunionstackcopybmv2l50_8") table tbl_headerunionstackcopybmv2l50_25 {
        actions = {
            headerunionstackcopybmv2l50_27();
        }
        const default_action = headerunionstackcopybmv2l50_27();
    }
    @hidden @name("tbl_headerunionstackcopybmv2l50_9") table tbl_headerunionstackcopybmv2l50_26 {
        actions = {
            headerunionstackcopybmv2l50_24();
        }
        const default_action = headerunionstackcopybmv2l50_24();
    }
    @hidden @name("tbl_headerunionstackcopybmv2l50_10") table tbl_headerunionstackcopybmv2l50_27 {
        actions = {
            headerunionstackcopybmv2l50_25();
        }
        const default_action = headerunionstackcopybmv2l50_25();
    }
    @hidden @name("tbl_headerunionstackcopybmv2l50_11") table tbl_headerunionstackcopybmv2l50_28 {
        actions = {
            headerunionstackcopybmv2l50_26();
        }
        const default_action = headerunionstackcopybmv2l50_26();
    }
    @hidden @name("tbl_headerunionstackcopybmv2l50_12") table tbl_headerunionstackcopybmv2l50_29 {
        actions = {
            headerunionstackcopybmv2l50_31();
        }
        const default_action = headerunionstackcopybmv2l50_31();
    }
    @hidden @name("tbl_headerunionstackcopybmv2l50_13") table tbl_headerunionstackcopybmv2l50_30 {
        actions = {
            headerunionstackcopybmv2l50_28();
        }
        const default_action = headerunionstackcopybmv2l50_28();
    }
    @hidden @name("tbl_headerunionstackcopybmv2l50_14") table tbl_headerunionstackcopybmv2l50_31 {
        actions = {
            headerunionstackcopybmv2l50_29();
        }
        const default_action = headerunionstackcopybmv2l50_29();
    }
    @hidden @name("tbl_headerunionstackcopybmv2l50_15") table tbl_headerunionstackcopybmv2l50_32 {
        actions = {
            headerunionstackcopybmv2l50_30();
        }
        const default_action = headerunionstackcopybmv2l50_30();
    }
    @hidden @name("tbl_headerunionstackcopybmv2l51") table tbl_headerunionstackcopybmv2l51_0 {
        actions = {
            headerunionstackcopybmv2l51_0();
        }
        const default_action = headerunionstackcopybmv2l51_0();
    }
    apply {
        tbl_headerunionstackcopybmv2l45_0.apply();
        if (hsiVar_0 == 1w0) {
            tbl_headerunionstackcopybmv2l50_16.apply();
            if (hsiVar_0 == 1w0) {
                tbl_headerunionstackcopybmv2l50_17.apply();
            } else if (hsiVar_0 == 1w1) {
                tbl_headerunionstackcopybmv2l50_18.apply();
            } else if (hsiVar_0 >= 1w1) {
                tbl_headerunionstackcopybmv2l50_19.apply();
            }
        } else if (hsiVar_0 == 1w1) {
            tbl_headerunionstackcopybmv2l50_20.apply();
            if (hsiVar_0 == 1w0) {
                tbl_headerunionstackcopybmv2l50_21.apply();
            } else if (hsiVar_0 == 1w1) {
                tbl_headerunionstackcopybmv2l50_22.apply();
            } else if (hsiVar_0 >= 1w1) {
                tbl_headerunionstackcopybmv2l50_23.apply();
            }
        }
        tbl_headerunionstackcopybmv2l50_24.apply();
        if (hsiVar_0 == 1w0) {
            tbl_headerunionstackcopybmv2l50_25.apply();
            if (hsiVar_0 == 1w0) {
                tbl_headerunionstackcopybmv2l50_26.apply();
            } else if (hsiVar_0 == 1w1) {
                tbl_headerunionstackcopybmv2l50_27.apply();
            } else if (hsiVar_0 >= 1w1) {
                tbl_headerunionstackcopybmv2l50_28.apply();
            }
        } else if (hsiVar_0 == 1w1) {
            tbl_headerunionstackcopybmv2l50_29.apply();
            if (hsiVar_0 == 1w0) {
                tbl_headerunionstackcopybmv2l50_30.apply();
            } else if (hsiVar_0 == 1w1) {
                tbl_headerunionstackcopybmv2l50_31.apply();
            } else if (hsiVar_0 >= 1w1) {
                tbl_headerunionstackcopybmv2l50_32.apply();
            }
        }
        tbl_headerunionstackcopybmv2l51_0.apply();
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
