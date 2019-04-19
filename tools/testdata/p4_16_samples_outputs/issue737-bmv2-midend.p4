#include <core.p4>
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
    @hidden action act() {
        hdr.h.field = 64w3;
    }
    @hidden action act_0() {
        hdr.h.field = 64w5;
    }
    @hidden action act_1() {
        hdr.h.field = 64w4;
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    @hidden table tbl_act_0 {
        actions = {
            act_0();
        }
        const default_action = act_0();
    }
    @hidden table tbl_act_1 {
        actions = {
            act_1();
        }
        const default_action = act_1();
    }
    apply {
        if (m.metafield) 
            tbl_act.apply();
        if (m.metafield == false) 
            tbl_act_0.apply();
        if (!m.metafield) 
            tbl_act_1.apply();
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

