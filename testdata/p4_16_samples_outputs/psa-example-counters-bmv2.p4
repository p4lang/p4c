#include <core.p4>
#include <psa.p4>

typedef bit<48> EthernetAddress;
header ethernet_t {
    EthernetAddress dstAddr;
    EthernetAddress srcAddr;
    bit<16>         etherType;
}

header ipv4_t {
    bit<4>  version;
    bit<4>  ihl;
    bit<8>  diffserv;
    bit<16> totalLen;
    bit<16> identification;
    bit<3>  flags;
    bit<13> fragOffset;
    bit<8>  ttl;
    bit<8>  protocol;
    bit<16> hdrChecksum;
    bit<32> srcAddr;
    bit<32> dstAddr;
}

struct empty_metadata_t {
}

struct fwd_metadata_t {
}

struct metadata {
    fwd_metadata_t fwd_metadata;
}

typedef bit<48> ByteCounter_t;
typedef bit<32> PacketCounter_t;
typedef bit<80> PacketByteCounter_t;
const bit<32> NUM_PORTS = 512;
struct headers {
    ethernet_t ethernet;
    ipv4_t     ipv4;
}

parser CommonParser(packet_in buffer, out headers parsed_hdr, inout metadata user_meta) {
    state start {
        transition parse_ethernet;
    }
    state parse_ethernet {
        buffer.extract(parsed_hdr.ethernet);
        transition select(parsed_hdr.ethernet.etherType) {
            0x800: parse_ipv4;
            default: accept;
        }
    }
    state parse_ipv4 {
        buffer.extract(parsed_hdr.ipv4);
        transition accept;
    }
}

parser IngressParserImpl(packet_in buffer, out headers parsed_hdr, inout metadata user_meta, in psa_ingress_parser_input_metadata_t istd, in empty_metadata_t resubmit_meta, in empty_metadata_t recirculate_meta) {
    CommonParser() p;
    state start {
        p.apply(buffer, parsed_hdr, user_meta);
        transition accept;
    }
}

parser EgressParserImpl(packet_in buffer, out headers parsed_hdr, inout metadata user_meta, in psa_egress_parser_input_metadata_t istd, in empty_metadata_t normal_meta, in empty_metadata_t clone_i2e_meta, in empty_metadata_t clone_e2e_meta) {
    CommonParser() p;
    state start {
        p.apply(buffer, parsed_hdr, user_meta);
        transition accept;
    }
}

control ingress(inout headers hdr, inout metadata user_meta, in psa_ingress_input_metadata_t istd, inout psa_ingress_output_metadata_t ostd) {
    Counter<ByteCounter_t, PortId_t>(NUM_PORTS, PSA_CounterType_t.BYTES) port_bytes_in;
    DirectCounter<PacketByteCounter_t>(PSA_CounterType_t.PACKETS_AND_BYTES) per_prefix_pkt_byte_count;
    action next_hop(PortId_t oport) {
        per_prefix_pkt_byte_count.count();
        send_to_port(ostd, oport);
    }
    action default_route_drop() {
        per_prefix_pkt_byte_count.count();
        ingress_drop(ostd);
    }
    table ipv4_da_lpm {
        key = {
            hdr.ipv4.dstAddr: lpm;
        }
        actions = {
            next_hop;
            default_route_drop;
        }
        default_action = default_route_drop;
        psa_direct_counter = per_prefix_pkt_byte_count;
    }
    apply {
        port_bytes_in.count(istd.ingress_port);
        if (hdr.ipv4.isValid()) {
            ipv4_da_lpm.apply();
        }
    }
}

control egress(inout headers hdr, inout metadata user_meta, in psa_egress_input_metadata_t istd, inout psa_egress_output_metadata_t ostd) {
    Counter<ByteCounter_t, PortId_t>(NUM_PORTS, PSA_CounterType_t.BYTES) port_bytes_out;
    apply {
        port_bytes_out.count(istd.egress_port);
    }
}

control CommonDeparserImpl(packet_out packet, inout headers hdr) {
    apply {
        packet.emit(hdr.ethernet);
        packet.emit(hdr.ipv4);
    }
}

control IngressDeparserImpl(packet_out buffer, out empty_metadata_t clone_i2e_meta, out empty_metadata_t resubmit_meta, out empty_metadata_t normal_meta, inout headers hdr, in metadata meta, in psa_ingress_output_metadata_t istd) {
    CommonDeparserImpl() cp;
    apply {
        cp.apply(buffer, hdr);
    }
}

control EgressDeparserImpl(packet_out buffer, out empty_metadata_t clone_e2e_meta, out empty_metadata_t recirculate_meta, inout headers hdr, in metadata meta, in psa_egress_output_metadata_t istd, in psa_egress_deparser_input_metadata_t edstd) {
    CommonDeparserImpl() cp;
    apply {
        cp.apply(buffer, hdr);
    }
}

IngressPipeline(IngressParserImpl(), ingress(), IngressDeparserImpl()) ip;

EgressPipeline(EgressParserImpl(), egress(), EgressDeparserImpl()) ep;

PSA_Switch(ip, PacketReplicationEngine(), ep, BufferingQueueingEngine()) main;

