#include <core.p4>
#include <bmv2/psa.p4>

struct EMPTY {
}

header ethernet_t {
    bit<8>  x0;
    bit<4>  x1;
    bit<8>  x2;
    bit<4>  x3;
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> ether_type;
}

struct headers_t {
    ethernet_t ethernet;
}

struct user_meta_data_t {
    bit<48> addr;
    bit<8>  x2;
    bit<8>  flg;
}

parser MyIngressParser(packet_in pkt, out headers_t hdr, inout user_meta_data_t m, in psa_ingress_parser_input_metadata_t c, in EMPTY d, in EMPTY e) {
    state start {
        pkt.extract<ethernet_t>(hdr.ethernet);
        transition accept;
    }
}

control MyIngressControl(inout headers_t hdr, inout user_meta_data_t m, in psa_ingress_input_metadata_t c, inout psa_ingress_output_metadata_t d) {
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("MyIngressControl.macswp") action macswp() {
        if (m.flg == 8w0x2) {
            m.x2 = hdr.ethernet.x2;
            m.addr = hdr.ethernet.dst_addr;
            hdr.ethernet.dst_addr = hdr.ethernet.src_addr;
            hdr.ethernet.src_addr = m.addr;
        }
    }
    @name("MyIngressControl.stub") table stub_0 {
        actions = {
            macswp();
            @defaultonly NoAction_1();
        }
        size = 1000000;
        default_action = NoAction_1();
    }
    @hidden action psadpdkhdrfieldnonalign70() {
        m.flg = 8w0x2;
        d.egress_port = (bit<32>)c.ingress_port ^ 32w1;
    }
    @hidden table tbl_psadpdkhdrfieldnonalign70 {
        actions = {
            psadpdkhdrfieldnonalign70();
        }
        const default_action = psadpdkhdrfieldnonalign70();
    }
    apply {
        tbl_psadpdkhdrfieldnonalign70.apply();
        stub_0.apply();
    }
}

control MyIngressDeparser(packet_out pkt, out EMPTY a, out EMPTY b, out EMPTY c, inout headers_t hdr, in user_meta_data_t e, in psa_ingress_output_metadata_t f) {
    @hidden action psadpdkhdrfieldnonalign85() {
        pkt.emit<ethernet_t>(hdr.ethernet);
    }
    @hidden table tbl_psadpdkhdrfieldnonalign85 {
        actions = {
            psadpdkhdrfieldnonalign85();
        }
        const default_action = psadpdkhdrfieldnonalign85();
    }
    apply {
        tbl_psadpdkhdrfieldnonalign85.apply();
    }
}

parser MyEgressParser(packet_in pkt, out EMPTY a, inout EMPTY b, in psa_egress_parser_input_metadata_t c, in EMPTY d, in EMPTY e, in EMPTY f) {
    state start {
        transition accept;
    }
}

control MyEgressControl(inout EMPTY a, inout EMPTY b, in psa_egress_input_metadata_t c, inout psa_egress_output_metadata_t d) {
    apply {
    }
}

control MyEgressDeparser(packet_out pkt, out EMPTY a, out EMPTY b, inout EMPTY c, in EMPTY d, in psa_egress_output_metadata_t e, in psa_egress_deparser_input_metadata_t f) {
    apply {
    }
}

IngressPipeline<headers_t, user_meta_data_t, EMPTY, EMPTY, EMPTY, EMPTY>(MyIngressParser(), MyIngressControl(), MyIngressDeparser()) ip;
EgressPipeline<EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY>(MyEgressParser(), MyEgressControl(), MyEgressDeparser()) ep;
PSA_Switch<headers_t, user_meta_data_t, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY>(ip, PacketReplicationEngine(), ep, BufferingQueueingEngine()) main;
