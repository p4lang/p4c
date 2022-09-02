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
    bit<8>  _s_f80;
    bit<16> _s_f161;
    bit<32> _f322;
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
    @hidden action p4rt_digest_complex63() {
        d.egress_port = c.ingress_port;
    }
    @hidden table tbl_p4rt_digest_complex63 {
        actions = {
            p4rt_digest_complex63();
        }
        const default_action = p4rt_digest_complex63();
    }
    apply {
        tbl_p4rt_digest_complex63.apply();
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
    s2_t_0 tmp_s2;
    s3_t_0 tmp_s3;
    @name("MyID.digest") Digest<digest_t>() digest_0;
    @hidden action p4rt_digest_complex92() {
        tmp_s2.f16 = 16w10;
        tmp_s2.f = 32w20;
        tmp_s3.f = 64w30;
        digest_0.pack((digest_t){h = hdr.h,port = f.egress_port,s2 = tmp_s2,s3 = tmp_s3});
    }
    @hidden table tbl_p4rt_digest_complex92 {
        actions = {
            p4rt_digest_complex92();
        }
        const default_action = p4rt_digest_complex92();
    }
    apply {
        tbl_p4rt_digest_complex92.apply();
    }
}

control MyED(packet_out buffer, out EMPTY a, out EMPTY b, inout EMPTY c, in EMPTY d, in psa_egress_output_metadata_t e, in psa_egress_deparser_input_metadata_t f) {
    apply {
    }
}

IngressPipeline<headers, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY>(MyIP(), MyIC(), MyID()) ip;

EgressPipeline<EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY>(MyEP(), MyEC(), MyED()) ep;

PSA_Switch<headers, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY>(ip, PacketReplicationEngine(), ep, BufferingQueueingEngine()) main;

