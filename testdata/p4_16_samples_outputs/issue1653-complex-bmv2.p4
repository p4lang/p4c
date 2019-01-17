#include <core.p4>
#include <v1model.p4>

enum bit<16> EthTypes {
    IPv4 = 0x800,
    ARP = 0x806,
    RARP = 0x8035,
    EtherTalk = 0x809b,
    VLAN = 0x8100,
    IPX = 0x8137,
    IPv6 = 0x86dd
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
        pk.extract(h.bvh0);
        pk.extract(h.bvh1);
        transition accept;
    }
}

control ingress(inout parsed_packet_t h, inout local_metadata_t local_metadata, inout standard_metadata_t standard_metadata) {
    bitvec_hdr bh;
    action do_act() {
        h.bvh1.row.alt1.valid = 0;
        local_metadata.row0.alt0.valid = 0;
    }
    table tns {
        key = {
            h.bvh1.row.alt1.valid         : exact;
            local_metadata.row0.alt0.valid: exact;
        }
        actions = {
            do_act;
        }
    }
    apply {
        tns.apply();
        bh.row.alt0.useHash = h.bvh0.row.alt0.useHash;
        bh.row.alt1.type = EthTypes.IPv4;
        h.bvh0.row.alt1.type = bh.row.alt1.type;
        local_metadata.row0.alt0.useHash = true;
        clone3(CloneType.I2E, 0, local_metadata.row0);
    }
}

control egress(inout parsed_packet_t hdr, inout local_metadata_t local_metadata, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control deparser(packet_out b, in parsed_packet_t h) {
    apply {
        b.emit(h.bvh0);
        b.emit(h.bvh1);
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

V1Switch(parse(), verify_checksum(), ingress(), egress(), compute_checksum(), deparser()) main;

