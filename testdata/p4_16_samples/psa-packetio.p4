#include <core.p4>
#include <psa.p4>

#ifndef CPU_PORT
#define CPU_PORT 320
#endif

typedef bit<9> PortId;

// ---------------------- HEADERS ----------------------
@controller_header("packet_in")
header packet_in_header_t {
    PortId ingress_port;
    bit<7> _padding0;
}

@not_extracted_in_egress
@controller_header("packet_out")
header packet_out_header_t {
    PortId egress_port;
    bit<7> _padding0;
}

struct metadata_t {
}

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

struct headers_t {
    ethernet_t ethernet;
    packet_out_header_t packet_out;
    packet_in_header_t packet_in;
}

struct empty_t {

}

// -------------------- PARSER ------------------------

parser IngressParserImpl(packet_in packet, out headers_t hdr, inout metadata_t meta,
    in psa_ingress_parser_input_metadata_t istd,
    in empty_t resub_meta, in empty_t recirc_meta
) {
    state parse_packet_out {
        packet.extract(hdr.packet_out);
        transition parse_ethernet;
    }

    state parse_ethernet {
        packet.extract(hdr.ethernet);
        transition accept;
    }

    state start {
        transition select(istd.ingress_port) {
            CPU_PORT: parse_packet_out;
            default: parse_ethernet;
        }
    }
}

// ------------------- CONTROLS ------------------

control IngressImpl(inout headers_t hdr, inout metadata_t meta,
    in psa_ingress_input_metadata_t istd,
    inout psa_ingress_output_metadata_t ostd
) {
    action send_to_cpu() {
        ostd.egress_port = CPU_PORT;
    }
    
    action set_egress_port(PortId port) {
        ostd.egress_port = port;
    }
    table table0 {
        /*
        Disabling timeout here as P4runtime doesn't allow setting timeouts.
        This way the FlowRuleTranslator will produce instances of PiTableEntry without timeout.
        */
        support_timeout = false;
        key = {
            istd.ingress_port              : ternary;
            hdr.ethernet.dstAddr           : ternary;
            hdr.ethernet.srcAddr           : ternary;
            hdr.ethernet.etherType         : ternary;
        }
        actions = {
            set_egress_port();
            send_to_cpu();
        }
        default_action = send_to_cpu();
    }
    apply {
        if (hdr.packet_out.isValid()) {
            ostd.egress_port = hdr.packet_out.egress_port;
        }
        else {
            table0.apply();
        }
    }
}

control IngressDeparserImpl(packet_out packet, out empty_t clone_i2e_meta,
    out empty_t resub_meta, out empty_t normal_meta, inout headers_t hdr,
    in metadata_t meta, in psa_ingress_output_metadata_t istd
) {
    apply {
        packet.emit(hdr.packet_in);
        packet.emit(hdr.ethernet);
    }
}

parser EgressParserImpl(packet_in packet, out headers_t hdr, inout metadata_t meta,
    in psa_egress_parser_input_metadata_t istd, in empty_t normal_meta,
    in empty_t clone_i2e_meta, in empty_t clone_e2e_meta
) {
    state parse_packet_out {
        packet.extract(hdr.packet_out);
        transition parse_ethernet;
    }

    state parse_ethernet {
        packet.extract(hdr.ethernet);
        transition accept;
    }

    state start {
        transition select(istd.egress_port) {
            CPU_PORT: parse_packet_out;
            default: parse_ethernet;
        }
    }
}

control EgressImpl(inout headers_t hdr, inout metadata_t meta,
    in psa_egress_input_metadata_t istd,
    inout psa_egress_output_metadata_t ostd
) {
    apply {
        hdr.packet_out.setInvalid();
        if (istd.egress_port == CPU_PORT) {
            hdr.packet_in.setValid();
            hdr.packet_in.ingress_port = istd.egress_port;
        }
    }
}

control EgressDeparserImpl(packet_out packet, out empty_t clone_e2e_meta,
    out empty_t recirc_meta, inout headers_t hdr, in metadata_t meta,
    in psa_egress_output_metadata_t istd, in psa_egress_deparser_input_metadata_t edstd
) {
    apply {
        packet.emit(hdr.packet_in);
        packet.emit(hdr.ethernet);
    }
}

IngressPipeline(IngressParserImpl(), IngressImpl(), IngressDeparserImpl()) ig;
EgressPipeline(EgressParserImpl(), EgressImpl(), EgressDeparserImpl()) eg;
PSA_Switch(ig, PacketReplicationEngine(), eg, BufferingQueueingEngine()) main;
