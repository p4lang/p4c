#include <core.p4>
#include <bmv2/psa.p4>

struct EMPTY {
}

struct s_t {
    bit<8>  f8;
    bit<16> f16;
}

struct s2_t<T> {
    bit<16> f16;
    T       f;
}

@name("s3_t") struct s3_t<T> {
    T f;
}

header h_t {
    s_t     s;
    bit<32> f32;
}

struct headers {
    h_t h;
}

parser MyIP(packet_in buffer, out headers hdr, inout EMPTY b, in psa_ingress_parser_input_metadata_t c, in EMPTY d, in EMPTY e) {
    state start {
        buffer.extract<h_t>(hdr.h);
        transition accept;
    }
}

parser MyEP(packet_in buffer, out EMPTY a, inout EMPTY b, in psa_egress_parser_input_metadata_t c, in EMPTY d, in EMPTY e, in EMPTY f) {
    state start {
        transition accept;
    }
}

control MyIC(inout headers hdr, inout EMPTY b, in psa_ingress_input_metadata_t c, inout psa_ingress_output_metadata_t d) {
    apply {
        d.egress_port = c.ingress_port;
    }
}

control MyEC(inout EMPTY a, inout EMPTY b, in psa_egress_input_metadata_t c, inout psa_egress_output_metadata_t d) {
    apply {
    }
}

struct s2_t_0 {
    bit<16> f16;
    bit<32> f;
}

@name("s3_t") struct s3_t_0 {
    bit<64> f;
}

struct digest_t {
    h_t      h;
    PortId_t port;
    s2_t_0   s2;
    s3_t_0   s3;
}

control MyID(packet_out buffer, out EMPTY a, out EMPTY b, out EMPTY c, inout headers hdr, in EMPTY e, in psa_ingress_output_metadata_t f) {
    Digest<digest_t>() digest;
    apply {
        digest.pack((digest_t){h = hdr.h,port = f.egress_port,s2 = (s2_t_0){f16 = 16w10,f = 32w20},s3 = (s3_t_0){f = 64w30}});
    }
}

control MyED(packet_out buffer, out EMPTY a, out EMPTY b, inout EMPTY c, in EMPTY d, in psa_egress_output_metadata_t e, in psa_egress_deparser_input_metadata_t f) {
    apply {
    }
}

IngressPipeline<headers, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY>(MyIP(), MyIC(), MyID()) ip;

EgressPipeline<EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY>(MyEP(), MyEC(), MyED()) ep;

PSA_Switch<headers, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY>(ip, PacketReplicationEngine(), ep, BufferingQueueingEngine()) main;

