#include <core.p4>
#include <dpdk/psa.p4>

struct EMPTY {
}

header h_t {
    bit<8> f;
}

struct headers_t {
    h_t u_h;
    h_t u_other;
}

parser MyIP(packet_in buffer, out headers_t hdr, inout EMPTY b, in psa_ingress_parser_input_metadata_t c, in EMPTY d, in EMPTY e) {
    @name("ih_h") h_t ih_h_0;
    @name("ih_other") h_t ih_other_0;
    state start {
        hdr.u_h.setValid();
        hdr.u_other.setInvalid();
        transition select(ih_h_0.isValid()) {
            true: start_true;
            false: start_false;
        }
    }
    state start_true {
        hdr.u_h.setValid();
        hdr.u_h = ih_h_0;
        hdr.u_other.setInvalid();
        transition start_join;
    }
    state start_false {
        hdr.u_h.setInvalid();
        transition start_join;
    }
    state start_join {
        transition select(ih_other_0.isValid()) {
            true: start_true_0;
            false: start_false_0;
        }
    }
    state start_true_0 {
        hdr.u_other.setValid();
        hdr.u_other = ih_other_0;
        hdr.u_h.setInvalid();
        transition start_join_0;
    }
    state start_false_0 {
        hdr.u_other.setInvalid();
        transition start_join_0;
    }
    state start_join_0 {
        transition accept;
    }
}

parser MyEP(packet_in buffer, out EMPTY a, inout EMPTY b, in psa_egress_parser_input_metadata_t c, in EMPTY d, in EMPTY e, in EMPTY f) {
    state start {
        transition accept;
    }
}

control MyIC(inout headers_t hdr, inout EMPTY b, in psa_ingress_input_metadata_t c, inout psa_ingress_output_metadata_t d) {
    apply {
    }
}

control MyEC(inout EMPTY a, inout EMPTY b, in psa_egress_input_metadata_t c, inout psa_egress_output_metadata_t d) {
    apply {
    }
}

control MyID(packet_out buffer, out EMPTY a, out EMPTY b, out EMPTY c, inout headers_t hdr, in EMPTY e, in psa_ingress_output_metadata_t f) {
    @hidden @name("psaissue5875union77") action psaissue5875union77_0() {
        buffer.emit<h_t>(hdr.u_h);
        buffer.emit<h_t>(hdr.u_other);
    }
    @hidden @name("tbl_psaissue5875union77") table tbl_psaissue5875union77_0 {
        actions = {
            psaissue5875union77_0();
        }
        const default_action = psaissue5875union77_0();
    }
    apply {
        tbl_psaissue5875union77_0.apply();
    }
}

control MyED(packet_out buffer, out EMPTY a, out EMPTY b, inout EMPTY c, in EMPTY d, in psa_egress_output_metadata_t e, in psa_egress_deparser_input_metadata_t f) {
    apply {
    }
}

IngressPipeline<headers_t, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY>(MyIP(), MyIC(), MyID()) ip;
EgressPipeline<EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY>(MyEP(), MyEC(), MyED()) ep;
PSA_Switch<headers_t, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY>(ip, PacketReplicationEngine(), ep, BufferingQueueingEngine()) main;
