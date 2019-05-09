#include <core.p4>
#include <v1model.p4>

enum bit<16> EthTypes {
    IPv4 = 16w0x800,
    ARP = 16w0x806,
    RARP = 16w0x8035,
    EtherTalk = 16w0x809b,
    VLAN = 16w0x8100,
    IPX = 16w0x8137,
    IPv6 = 16w0x86dd
}

struct alt_t {
    bit<1>   valid;
    bit<7>   port;
    int<8>   hashRes;
    bool     useHash;
    EthTypes type;
    bit<7>   pad;
}

struct row_t {
    alt_t alt0;
    alt_t alt1;
}

header bitvec_hdr {
    row_t row;
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
    bitvec_hdr bh_0;
    @name("ingress.do_act") action do_act() {
        h.bvh1.row.alt1.valid = 1w0;
        local_metadata.row0.alt0.valid = 1w0;
    }
    @name("ingress.tns") table tns_0 {
        key = {
            h.bvh1.row.alt1.valid         : exact @name("h.bvh1.row.alt1.valid") ;
            local_metadata.row0.alt0.valid: exact @name("local_metadata.row0.alt0.valid") ;
        }
        actions = {
            do_act();
            @defaultonly NoAction_0();
        }
        default_action = NoAction_0();
    }
    apply {
        tns_0.apply();
        bh_0.row.alt1.type = EthTypes.IPv4;
        h.bvh0.row.alt1.type = bh_0.row.alt1.type;
        local_metadata.row0.alt0.useHash = true;
        clone3<row_t>(CloneType.I2E, 32w0, local_metadata.row0);
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

