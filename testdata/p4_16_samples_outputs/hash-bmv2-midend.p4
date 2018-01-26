#include <core.p4>
#include <v1model.p4>

typedef standard_metadata_t std_meta_t;
header hash_t {
    bit<16> hash;
}

header ipv4_t {
    bit<32> lkp_ipv4_sa;
}

struct M {
    hash_t hash;
    ipv4_t ipv4;
}

struct H {
}

parser ParserI(packet_in pk, out H hdr, inout M meta, inout std_meta_t std_meta) {
    state start {
        transition accept;
    }
}

control VerifyChecksumI(inout H hdr, inout M meta) {
    apply {
    }
}

control ComputeChecksumI(inout H hdr, inout M meta) {
    apply {
    }
}

struct tuple_0 {
    bit<32> field;
}

control IngressI(inout H hdr, inout M meta, inout std_meta_t std_meta) {
    @name("IngressI.a") action a_0() {
        hash<bit<16>, bit<16>, tuple_0, bit<32>>(meta.hash.hash, HashAlgorithm.crc16, 16w0, { meta.ipv4.lkp_ipv4_sa }, 32w65536);
    }
    @hidden table tbl_a {
        actions = {
            a_0();
        }
        const default_action = a_0();
    }
    apply {
        tbl_a.apply();
    }
}

control EgressI(inout H hdr, inout M meta, inout std_meta_t std_meta) {
    apply {
    }
}

control DeparserI(packet_out b, in H hdr) {
    apply {
    }
}

V1Switch<H, M>(ParserI(), VerifyChecksumI(), IngressI(), EgressI(), ComputeChecksumI(), DeparserI()) main;

