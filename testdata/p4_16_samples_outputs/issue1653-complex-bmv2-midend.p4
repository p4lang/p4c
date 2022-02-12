#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

struct alt_t {
    bit<1>  valid;
    bit<7>  port;
    int<8>  hashRes;
    bool    useHash;
    bit<16> type;
    bit<7>  pad;
}

struct row_t {
    alt_t alt0;
    alt_t alt1;
}

header bitvec_hdr {
    bit<1>  _row_alt0_valid0;
    bit<7>  _row_alt0_port1;
    int<8>  _row_alt0_hashRes2;
    bool    _row_alt0_useHash3;
    bit<16> _row_alt0_type4;
    bit<7>  _row_alt0_pad5;
    bit<1>  _row_alt1_valid6;
    bit<7>  _row_alt1_port7;
    int<8>  _row_alt1_hashRes8;
    bool    _row_alt1_useHash9;
    bit<16> _row_alt1_type10;
    bit<7>  _row_alt1_pad11;
}

struct local_metadata_t {
    @field_list(0) 
    bit<1>     _row0_alt0_valid0;
    @field_list(0) 
    bit<7>     _row0_alt0_port1;
    @field_list(0) 
    int<8>     _row0_alt0_hashRes2;
    @field_list(0) 
    bool       _row0_alt0_useHash3;
    @field_list(0) 
    bit<16>    _row0_alt0_type4;
    @field_list(0) 
    bit<7>     _row0_alt0_pad5;
    @field_list(0) 
    bit<1>     _row0_alt1_valid6;
    @field_list(0) 
    bit<7>     _row0_alt1_port7;
    @field_list(0) 
    int<8>     _row0_alt1_hashRes8;
    @field_list(0) 
    bool       _row0_alt1_useHash9;
    @field_list(0) 
    bit<16>    _row0_alt1_type10;
    @field_list(0) 
    bit<7>     _row0_alt1_pad11;
    bit<1>     _row1_alt0_valid12;
    bit<7>     _row1_alt0_port13;
    int<8>     _row1_alt0_hashRes14;
    bool       _row1_alt0_useHash15;
    bit<16>    _row1_alt0_type16;
    bit<7>     _row1_alt0_pad17;
    bit<1>     _row1_alt1_valid18;
    bit<7>     _row1_alt1_port19;
    int<8>     _row1_alt1_hashRes20;
    bool       _row1_alt1_useHash21;
    bit<16>    _row1_alt1_type22;
    bit<7>     _row1_alt1_pad23;
    bitvec_hdr _bvh024;
    bitvec_hdr _bvh125;
}

struct parsed_packet_t {
    bitvec_hdr bvh0;
    bitvec_hdr bvh1;
}

parser parse(packet_in pk, out parsed_packet_t h, inout local_metadata_t local_metadata, inout standard_metadata_t standard_metadata) {
    state start {
        pk.extract<bitvec_hdr>(h.bvh0);
        pk.extract<bitvec_hdr>(h.bvh1);
        transition accept;
    }
}

control ingress(inout parsed_packet_t h, inout local_metadata_t local_metadata, inout standard_metadata_t standard_metadata) {
    @name("ingress.bh") bitvec_hdr bh_0;
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("ingress.do_act") action do_act() {
        h.bvh1._row_alt1_valid6 = 1w0;
        local_metadata._row0_alt0_valid0 = 1w0;
    }
    @name("ingress.tns") table tns_0 {
        key = {
            h.bvh1._row_alt1_valid6         : exact @name("h.bvh1.row.alt1.valid") ;
            local_metadata._row0_alt0_valid0: exact @name("local_metadata.row0.alt0.valid") ;
        }
        actions = {
            do_act();
            @defaultonly NoAction_1();
        }
        default_action = NoAction_1();
    }
    @hidden action issue1653complexbmv2l72() {
        bh_0.setInvalid();
    }
    @hidden action issue1653complexbmv2l95() {
        bh_0._row_alt1_type10 = 16w0x800;
        h.bvh0._row_alt1_type10 = 16w0x800;
        local_metadata._row0_alt0_useHash3 = true;
        clone_preserving_field_list(CloneType.I2E, 32w0, 8w0);
    }
    @hidden table tbl_issue1653complexbmv2l72 {
        actions = {
            issue1653complexbmv2l72();
        }
        const default_action = issue1653complexbmv2l72();
    }
    @hidden table tbl_issue1653complexbmv2l95 {
        actions = {
            issue1653complexbmv2l95();
        }
        const default_action = issue1653complexbmv2l95();
    }
    apply {
        tbl_issue1653complexbmv2l72.apply();
        tns_0.apply();
        tbl_issue1653complexbmv2l95.apply();
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

