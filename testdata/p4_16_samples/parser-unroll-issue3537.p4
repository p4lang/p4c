#include <core.p4>
#include <v1model.p4>

header h1_t {}
header h2_t {
    bit<16> f1;
}
header h3_t {
    bit<3> pad;
    bit<13> f2;
    bit<8> f3;
}
header h4_t {
    bit<8> f4;
}
header h5_t {}
header h6_t {
    bit<16> f5;
}

struct H {
    h1_t h1;
    h2_t h2;
    h3_t h3;
    h4_t h4;
    h5_t h5;
    h6_t h6;
};
struct M { };

parser ParserI(packet_in packet, out H hdr, inout M meta, inout standard_metadata_t smeta) {
    state start {
        packet.extract<h1_t>(hdr.h1);
        packet.extract<h2_t>(hdr.h2);
        transition select(hdr.h2.f1) {
            16w0x800: parse_ipv4;
            16w0x86dd: parse_ipv6;
            default: accept;
        }
    }
    state parse_ipv4 {
        packet.extract<h3_t>(hdr.h3);
        transition select(hdr.h3.f2, hdr.h3.f3) {
            (13w0, 8w6): parse_tcp;
            (13w0, 8w17): parse_udp;
            default: accept;
        }
    }
    state parse_ipv6 {
        packet.extract<h4_t>(hdr.h4);
        transition select(hdr.h4.f4) {
            8w6: parse_tcp;
            8w17: parse_udp;
            default: accept;
        }
    }
    state parse_tcp {
        packet.extract<h5_t>(hdr.h5);
        transition accept;
    }
    state parse_udp {
        packet.extract<h6_t>(hdr.h6);
        transition select(hdr.h6.f5) {
            16w0xcafe: parse_ipv4;
            16w0xffff: reject;
            default: accept;
        }
    }
}

control Aux(inout M meta) {
    apply {
    }
}

control IngressI(inout H hdr, inout M meta, inout standard_metadata_t smeta) {
    apply {
    }
}

control EgressI(inout H hdr, inout M meta, inout standard_metadata_t smeta) {
    apply { }
}

control DeparserI(packet_out pk, in H hdr) {
    apply { }
}

control VerifyChecksumI(inout H hdr, inout M meta) {
    apply { }
}

control ComputeChecksumI(inout H hdr, inout M meta) {
    apply { }
}


V1Switch(ParserI(), VerifyChecksumI(), IngressI(), EgressI(),
         ComputeChecksumI(), DeparserI()) main;
