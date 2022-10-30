#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header fixedLenHeader_h {
    bit<64> field;
}

struct Meta {
    bool metafield;
}

struct Parsed_packet {
    fixedLenHeader_h h;
}

parser TopParser(packet_in b, out Parsed_packet p, inout Meta m, inout standard_metadata_t metadata) {
    state start {
        m.metafield = true;
        transition accept;
    }
}

control VeryChecksum(inout Parsed_packet hdr, inout Meta meta) {
    apply {
    }
}

control IngressP(inout Parsed_packet hdr, inout Meta m, inout standard_metadata_t standard_metadata) {
    @hidden action issue737bmv2l35() {
        hdr.h.field = 64w3;
    }
    @hidden action issue737bmv2l38() {
        hdr.h.field = 64w5;
    }
    @hidden action issue737bmv2l41() {
        hdr.h.field = 64w4;
    }
    @hidden table tbl_issue737bmv2l35 {
        actions = {
            issue737bmv2l35();
        }
        const default_action = issue737bmv2l35();
    }
    @hidden table tbl_issue737bmv2l38 {
        actions = {
            issue737bmv2l38();
        }
        const default_action = issue737bmv2l38();
    }
    @hidden table tbl_issue737bmv2l41 {
        actions = {
            issue737bmv2l41();
        }
        const default_action = issue737bmv2l41();
    }
    apply {
        if (m.metafield) {
            tbl_issue737bmv2l35.apply();
        }
        if (m.metafield) {
            ;
        } else {
            tbl_issue737bmv2l38.apply();
        }
        if (m.metafield) {
            ;
        } else {
            tbl_issue737bmv2l41.apply();
        }
    }
}

control EgressP(inout Parsed_packet hdr, inout Meta meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control ChecksumComputer(inout Parsed_packet hdr, inout Meta meta) {
    apply {
    }
}

control TopDeparser(packet_out b, in Parsed_packet hdr) {
    apply {
    }
}

V1Switch<Parsed_packet, Meta>(TopParser(), VeryChecksum(), IngressP(), EgressP(), ChecksumComputer(), TopDeparser()) main;
