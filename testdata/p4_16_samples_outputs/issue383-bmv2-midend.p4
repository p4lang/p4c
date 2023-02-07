#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

struct alt_t {
    bit<1> valid;
    bit<7> port;
}

@MNK_annotation("(test flatten)") struct row_t {
    alt_t alt0;
    alt_t alt1;
}

header bitvec_hdr {
    bit<1> _row_alt0_valid0;
    bit<7> _row_alt0_port1;
    bit<1> _row_alt1_valid2;
    bit<7> _row_alt1_port3;
}

struct col_t {
    bitvec_hdr bvh;
}

struct local_metadata_t {
    @MNK_annotation("(test flatten)") @field_list(0)
    bit<1>     _row0_alt0_valid0;
    @MNK_annotation("(test flatten)") @field_list(0)
    bit<7>     _row0_alt0_port1;
    @MNK_annotation("(test flatten)") @field_list(0)
    bit<1>     _row0_alt1_valid2;
    @MNK_annotation("(test flatten)") @field_list(0)
    bit<7>     _row0_alt1_port3;
    @MNK_annotation("(test flatten)")
    bit<1>     _row1_alt0_valid4;
    @MNK_annotation("(test flatten)")
    bit<7>     _row1_alt0_port5;
    @MNK_annotation("(test flatten)")
    bit<1>     _row1_alt1_valid6;
    @MNK_annotation("(test flatten)")
    bit<7>     _row1_alt1_port7;
    bitvec_hdr _col_bvh8;
    bitvec_hdr _bvh09;
    bitvec_hdr _bvh110;
}

struct parsed_packet_t {
    bitvec_hdr bvh0;
    bitvec_hdr bvh1;
}

struct tst_t {
    row_t      row0;
    row_t      row1;
    col_t      col;
    bitvec_hdr bvh0;
    bitvec_hdr bvh1;
}

parser parse(packet_in pk, out parsed_packet_t h, inout local_metadata_t local_metadata, inout standard_metadata_t standard_metadata) {
    state start {
        pk.extract<bitvec_hdr>(h.bvh0);
        pk.extract<bitvec_hdr>(h.bvh1);
        pk.extract<bitvec_hdr>(local_metadata._col_bvh8);
        transition accept;
    }
}

control ingress(inout parsed_packet_t h, inout local_metadata_t local_metadata, inout standard_metadata_t standard_metadata) {
    bitvec_hdr s_0_col_bvh;
    bitvec_hdr s_0_bvh0;
    bitvec_hdr s_0_bvh1;
    @name("ingress.bh") bitvec_hdr bh_0;
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("ingress.do_act") action do_act() {
        h.bvh1._row_alt1_valid2 = 1w0;
    }
    @name("ingress.tns") table tns_0 {
        key = {
            h.bvh1._row_alt1_valid2                  : exact @name("h.bvh1.row.alt1.valid");
            local_metadata._col_bvh8._row_alt0_valid0: exact @name("local_metadata.col.bvh.row.alt0.valid");
        }
        actions = {
            do_act();
            @defaultonly NoAction_1();
        }
        default_action = NoAction_1();
    }
    @hidden action issue383bmv2l75() {
        s_0_col_bvh.setInvalid();
        s_0_bvh0.setInvalid();
        s_0_bvh1.setInvalid();
        bh_0.setInvalid();
    }
    @hidden action issue383bmv2l105() {
        local_metadata._col_bvh8._row_alt0_valid0 = 1w0;
        local_metadata._row0_alt0_valid0 = local_metadata._row1_alt1_valid6;
        local_metadata._row0_alt0_port1 = local_metadata._row1_alt1_port7;
        local_metadata._row1_alt0_valid4 = 1w1;
        local_metadata._row1_alt1_port7 = local_metadata._row0_alt1_port3 + 7w1;
        clone_preserving_field_list(CloneType.I2E, 32w1, 8w0);
    }
    @hidden table tbl_issue383bmv2l75 {
        actions = {
            issue383bmv2l75();
        }
        const default_action = issue383bmv2l75();
    }
    @hidden table tbl_issue383bmv2l105 {
        actions = {
            issue383bmv2l105();
        }
        const default_action = issue383bmv2l105();
    }
    apply {
        tbl_issue383bmv2l75.apply();
        tns_0.apply();
        tbl_issue383bmv2l105.apply();
    }
}

control egress(inout parsed_packet_t hdr, inout local_metadata_t local_metadata, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control deparser(packet_out b, in parsed_packet_t h) {
    apply {
        b.emit<bitvec_hdr>(h.bvh0);
        b.emit<bitvec_hdr>(h.bvh1);
    }
}

control verifyChecksum(inout parsed_packet_t hdr, inout local_metadata_t local_metadata) {
    apply {
    }
}

control compute_checksum(inout parsed_packet_t hdr, inout local_metadata_t local_metadata) {
    apply {
    }
}

V1Switch<parsed_packet_t, local_metadata_t>(parse(), verifyChecksum(), ingress(), egress(), compute_checksum(), deparser()) main;
