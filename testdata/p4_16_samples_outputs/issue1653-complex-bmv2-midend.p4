#include <core.p4>
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
    row_t      row0;
    row_t      row1;
    bitvec_hdr bvh0;
    bitvec_hdr bvh1;
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
    @name(".NoAction") action NoAction_0() {
    }
    @name("ingress.do_act") action do_act() {
        h.bvh1._row_alt1_valid6 = 1w0;
        local_metadata.row0.alt0.valid = 1w0;
    }
    @name("ingress.tns") table tns_0 {
        key = {
            h.bvh1._row_alt1_valid6       : exact @name("h.bvh1.row.alt1.valid") ;
            local_metadata.row0.alt0.valid: exact @name("local_metadata.row0.alt0.valid") ;
        }
        actions = {
            do_act();
            @defaultonly NoAction_0();
        }
        default_action = NoAction_0();
    }
    @hidden action act() {
        local_metadata.row0.alt0.useHash = true;
        clone3<row_t>(CloneType.I2E, 32w0, local_metadata.row0);
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    apply {
        tns_0.apply();
        tbl_act.apply();
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

control verify_checksum(inout parsed_packet_t hdr, inout local_metadata_t local_metadata) {
    apply {
    }
}

control compute_checksum(inout parsed_packet_t hdr, inout local_metadata_t local_metadata) {
    apply {
    }
}

V1Switch<parsed_packet_t, local_metadata_t>(parse(), verify_checksum(), ingress(), egress(), compute_checksum(), deparser()) main;

