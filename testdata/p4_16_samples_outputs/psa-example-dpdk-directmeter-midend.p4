#include <core.p4>
#include <dpdk/psa.p4>

struct EMPTY {
}

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

struct headers_t {
    ethernet_t ethernet;
}

struct metadata_t {
    bit<32> port_in;
    bit<32> port_out;
}

parser MyIP(packet_in buffer, out headers_t hdr, inout metadata_t b, in psa_ingress_parser_input_metadata_t c, in EMPTY d, in EMPTY e) {
    state start {
        buffer.extract<ethernet_t>(hdr.ethernet);
        transition accept;
    }
}

parser MyEP(packet_in buffer, out EMPTY a, inout metadata_t b, in psa_egress_parser_input_metadata_t c, in EMPTY d, in EMPTY e, in EMPTY f) {
    state start {
        transition accept;
    }
}

control MyIC(inout headers_t hdr, inout metadata_t b, in psa_ingress_input_metadata_t c, inout psa_ingress_output_metadata_t d) {
    @name("MyIC.color_out") PSA_MeterColor_t color_out_0;
    @name("MyIC.color_in") PSA_MeterColor_t color_in_0;
    @name("MyIC.tmp") bit<32> tmp;
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("MyIC.meter0") DirectMeter(PSA_MeterType_t.PACKETS) meter0_0;
    @name("MyIC.execute_meter") action execute_meter() {
        color_out_0 = meter0_0.dpdk_execute(color_in_0, 32w1024);
        if (color_out_0 == PSA_MeterColor_t.GREEN) {
            tmp = 32w1;
        } else {
            tmp = 32w0;
        }
        b.port_out = tmp;
    }
    @name("MyIC.tbl") table tbl_0 {
        key = {
            hdr.ethernet.srcAddr: exact @name("hdr.ethernet.srcAddr");
        }
        actions = {
            NoAction_1();
            execute_meter();
        }
        psa_direct_meter = meter0_0;
        default_action = NoAction_1();
    }
    @hidden action psaexampledpdkdirectmeter57() {
        color_in_0 = PSA_MeterColor_t.RED;
    }
    @hidden table tbl_psaexampledpdkdirectmeter57 {
        actions = {
            psaexampledpdkdirectmeter57();
        }
        const default_action = psaexampledpdkdirectmeter57();
    }
    apply {
        tbl_psaexampledpdkdirectmeter57.apply();
        tbl_0.apply();
    }
}

control MyEC(inout EMPTY a, inout metadata_t b, in psa_egress_input_metadata_t c, inout psa_egress_output_metadata_t d) {
    apply {
    }
}

control MyID(packet_out buffer, out EMPTY a, out EMPTY b, out EMPTY c, inout headers_t hdr, in metadata_t e, in psa_ingress_output_metadata_t f) {
    apply {
    }
}

control MyED(packet_out buffer, out EMPTY a, out EMPTY b, inout EMPTY c, in metadata_t d, in psa_egress_output_metadata_t e, in psa_egress_deparser_input_metadata_t f) {
    apply {
    }
}

IngressPipeline<headers_t, metadata_t, EMPTY, EMPTY, EMPTY, EMPTY>(MyIP(), MyIC(), MyID()) ip;
EgressPipeline<EMPTY, metadata_t, EMPTY, EMPTY, EMPTY, EMPTY>(MyEP(), MyEC(), MyED()) ep;
PSA_Switch<headers_t, metadata_t, EMPTY, metadata_t, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY>(ip, PacketReplicationEngine(), ep, BufferingQueueingEngine()) main;
