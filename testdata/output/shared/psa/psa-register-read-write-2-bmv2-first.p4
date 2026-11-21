#include <core.p4>
#include <bmv2/psa.p4>

struct EMPTY {
}

typedef bit<48> EthernetAddress;
header ethernet_t {
    EthernetAddress dstAddr;
    EthernetAddress srcAddr;
    bit<16>         etherType;
}

header output_data_t {
    bit<32> word0;
    bit<32> word1;
    bit<32> word2;
    bit<32> word3;
}

struct headers_t {
    ethernet_t    ethernet;
    output_data_t output_data;
}

struct metadata_t {
}

parser MyIP(packet_in pkt, out headers_t hdr, inout metadata_t user_meta, in psa_ingress_parser_input_metadata_t istd, in EMPTY resubmit_meta, in EMPTY recirculate_meta) {
    state start {
        pkt.extract<ethernet_t>(hdr.ethernet);
        pkt.extract<output_data_t>(hdr.output_data);
        transition accept;
    }
}

parser MyEP(packet_in pkt, out headers_t hdr, inout metadata_t user_meta, in psa_egress_parser_input_metadata_t istd, in EMPTY normal_meta, in EMPTY clone_i2e_meta, in EMPTY clone_e2e_meta) {
    state start {
        transition accept;
    }
}

control MyIC(inout headers_t hdr, inout metadata_t user_meta, in psa_ingress_input_metadata_t istd, inout psa_ingress_output_metadata_t ostd) {
    Register<bit<16>, bit<8>>(32w6) reg;
    bit<8> idx;
    bit<8> action_type;
    bit<16> orig_data;
    bit<16> next_data;
    apply {
        if (hdr.ethernet.isValid()) {
            idx = hdr.ethernet.dstAddr[7:0];
            action_type = hdr.ethernet.dstAddr[15:8];
            bool validAction = action_type >= 8w1 && action_type <= 8w3;
            if (validAction) {
                orig_data = reg.read(idx);
            }
            if (action_type == 8w1) {
                next_data = hdr.ethernet.dstAddr[47:32];
            } else if (action_type == 8w2) {
                next_data = orig_data;
            } else if (action_type == 8w3) {
                next_data = orig_data + 16w1;
            } else {
                orig_data = 16w0xdead;
                next_data = 16w0xbeef;
            }
            reg.write(idx, next_data);
            hdr.output_data.word0 = (bit<32>)orig_data;
            hdr.output_data.word1 = (bit<32>)next_data;
        }
        send_to_port(ostd, (PortId_t)32w1);
    }
}

control MyEC(inout headers_t hdr, inout metadata_t user_meta, in psa_egress_input_metadata_t istd, inout psa_egress_output_metadata_t ostd) {
    apply {
    }
}

control MyID(packet_out pkt, out EMPTY clone_i2e_meta, out EMPTY resubmit_meta, out EMPTY normal_meta, inout headers_t hdr, in metadata_t user_meta, in psa_ingress_output_metadata_t istd) {
    apply {
        pkt.emit<ethernet_t>(hdr.ethernet);
        pkt.emit<output_data_t>(hdr.output_data);
    }
}

control MyED(packet_out pkt, out EMPTY clone_e2e_meta, out EMPTY recirculate_meta, inout headers_t hdr, in metadata_t user_meta, in psa_egress_output_metadata_t istd, in psa_egress_deparser_input_metadata_t edstd) {
    apply {
    }
}

IngressPipeline<headers_t, metadata_t, EMPTY, EMPTY, EMPTY, EMPTY>(MyIP(), MyIC(), MyID()) ip;
EgressPipeline<headers_t, metadata_t, EMPTY, EMPTY, EMPTY, EMPTY>(MyEP(), MyEC(), MyED()) ep;
PSA_Switch<headers_t, metadata_t, headers_t, metadata_t, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY>(ip, PacketReplicationEngine(), ep, BufferingQueueingEngine()) main;
