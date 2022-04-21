#include <core.p4>
#include <bmv2/psa.p4>

struct EMPTY {
}

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> ether_type;
}

struct headers_t {
    ethernet_t ethernet;
}

struct user_meta_data_t {
    bit<48> addr;
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
    @name("MyIngressControl.nonDefAct") action nonDefAct() {
        m.addr = hdr.ethernet.dst_addr;
        hdr.ethernet.dst_addr = hdr.ethernet.src_addr;
        hdr.ethernet.src_addr = m.addr;
    }
    @name("MyIngressControl.macswp") action macswp(@name("tmp1") bit<32> tmp1, @name("tmp2") bit<32> tmp2) {
        m.addr = (tmp1 == 32w0x1 && tmp2 == 32w0x2 ? hdr.ethernet.dst_addr : m.addr);
        hdr.ethernet.dst_addr = (tmp1 == 32w0x1 && tmp2 == 32w0x2 ? hdr.ethernet.src_addr : hdr.ethernet.dst_addr);
        hdr.ethernet.src_addr = (tmp1 == 32w0x1 && tmp2 == 32w0x2 ? m.addr : hdr.ethernet.src_addr);
    }
    @name("MyIngressControl.stub") table stub_0 {
        key = {
        }
        actions = {
            macswp();
            nonDefAct();
            NoAction_1();
        }
        const default_action = NoAction_1();
        size = 1000000;
    }
    @hidden action psadpdknonzeroargdefaultaction05l70() {
        d.egress_port = (bit<32>)c.ingress_port ^ 32w1;
    }
    @hidden table tbl_psadpdknonzeroargdefaultaction05l70 {
        actions = {
            psadpdknonzeroargdefaultaction05l70();
        }
        const default_action = psadpdknonzeroargdefaultaction05l70();
    }
    apply {
        tbl_psadpdknonzeroargdefaultaction05l70.apply();
        stub_0.apply();
    }
}

control MyIngressDeparser(packet_out pkt, out EMPTY a, out EMPTY b, out EMPTY c, inout headers_t hdr, in user_meta_data_t e, in psa_ingress_output_metadata_t f) {
    @hidden action psadpdknonzeroargdefaultaction05l85() {
        pkt.emit<ethernet_t>(hdr.ethernet);
    }
    @hidden table tbl_psadpdknonzeroargdefaultaction05l85 {
        actions = {
            psadpdknonzeroargdefaultaction05l85();
        }
        const default_action = psadpdknonzeroargdefaultaction05l85();
    }
    apply {
        tbl_psadpdknonzeroargdefaultaction05l85.apply();
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

