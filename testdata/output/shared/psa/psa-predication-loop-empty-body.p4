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

struct headers_t {
    ethernet_t ethernet;
}

struct metadata_t {
    bit<32> counter;
}

parser MyIP(packet_in buffer, out headers_t hdr, inout metadata_t meta, in psa_ingress_parser_input_metadata_t c, in EMPTY d, in EMPTY e) {
    state start {
        buffer.extract(hdr.ethernet);
        meta.counter = 0;
        transition accept;
    }
}

parser MyEP(packet_in buffer, out EMPTY a, inout EMPTY b, in psa_egress_parser_input_metadata_t c, in EMPTY d, in EMPTY e, in EMPTY f) {
    state start {
        transition accept;
    }
}

control MyIC(inout headers_t hdr, inout metadata_t meta, in psa_ingress_input_metadata_t c, inout psa_ingress_output_metadata_t d) {
    action increment_in_loop() {
        if (meta.counter == 0) {
            for (bit<32> i = 0; i < 10; i = i + 1) {
                meta.counter = meta.counter + 1;
            }
        }
    }
    table tbl {
        key = {
            hdr.ethernet.srcAddr: exact;
        }
        actions = {
            NoAction;
            increment_in_loop;
        }
    }
    apply {
        tbl.apply();
    }
}

control MyEC(inout EMPTY a, inout EMPTY b, in psa_egress_input_metadata_t c, inout psa_egress_output_metadata_t d) {
    apply {
    }
}

control MyID(packet_out buffer, out EMPTY a, out EMPTY b, out EMPTY c, inout headers_t hdr, in metadata_t meta, in psa_ingress_output_metadata_t f) {
    apply {
        buffer.emit(hdr.ethernet);
    }
}

control MyED(packet_out buffer, out EMPTY a, out EMPTY b, inout EMPTY c, in EMPTY d, in psa_egress_output_metadata_t e, in psa_egress_deparser_input_metadata_t f) {
    apply {
    }
}

IngressPipeline(MyIP(), MyIC(), MyID()) ip;
EgressPipeline(MyEP(), MyEC(), MyED()) ep;
PSA_Switch(ip, PacketReplicationEngine(), ep, BufferingQueueingEngine()) main;
