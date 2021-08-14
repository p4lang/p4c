#include <core.p4>
#include <bmv2/psa.p4>

const bit<8> GTP_V1 = 0x1;
typedef bit<48> mac_addr_t;
typedef bit<32> ipv4_addr_t;
typedef bit<32> teid_t;
enum bit<16> EtherType {
    IPV4 = 0x800,
    IPV6 = 0x86dd
}

enum bit<8> IPProtocol {
    ICMP = 1,
    TCP = 6,
    UDP = 17
}

enum bit<8> GTPUMessageType {
    GPDU = 255
}

enum bit<16> L4Port {
    GTP_GPDU = 2152
}

struct empty_metadata_t {
}

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> ether_type;
}

header ipv4_t {
    bit<8>      ver_ihl;
    bit<8>      diffserv;
    bit<16>     total_len;
    bit<16>     identification;
    bit<16>     flags_offset;
    bit<8>      ttl;
    IPProtocol  protocol;
    bit<16>     hdr_checksum;
    ipv4_addr_t src_addr;
    ipv4_addr_t dst_addr;
}

header udp_t {
    bit<16> src_port;
    bit<16> dst_port;
    bit<16> length;
    bit<16> checksum;
}

header gtpu_t {
    bit<8>          flags;
    GTPUMessageType msgtype;
    bit<16>         msglen;
    teid_t          teid;
}

struct headers_t {
    ethernet_t ethernet;
    ipv4_t     ipv4;
    udp_t      udp;
    gtpu_t     gtpu;
}

struct local_metadata_t {
    bit<48> dst_mac;
    bit<48> src_mac;
    bit<32> src_addr;
    bit<32> dst_addr;
    bit<16> l4_sport;
    bit<16> l4_dport;
}

parser packet_parser(packet_in packet, out headers_t headers, inout local_metadata_t local_metadata, in psa_ingress_parser_input_metadata_t istd, in empty_metadata_t resub_meta, in empty_metadata_t recirc_meta) {
    state start {
        transition parse_ethernet;
    }
    state parse_ethernet {
        packet.extract(headers.ethernet);
        transition select(headers.ethernet.ether_type) {
            EtherType.IPV4: parse_ipv4;
            default: accept;
        }
    }
    state parse_ipv4 {
        packet.extract(headers.ipv4);
        transition select(headers.ipv4.protocol) {
            IPProtocol.UDP: parse_udp;
            default: accept;
        }
    }
    state parse_udp {
        packet.extract(headers.udp);
        gtpu_t gtpu = packet.lookahead<gtpu_t>();
        bit<8> ver = gtpu.flags & 0xfc;
        transition select(headers.udp.dst_port, ver) {
            (L4Port.GTP_GPDU, 0x4): parse_gtpu;
            default: accept;
        }
    }
    state parse_gtpu {
        packet.extract(headers.gtpu);
        transition accept;
    }
}

control packet_deparser(packet_out packet, out empty_metadata_t clone_i2e_meta, out empty_metadata_t resubmit_meta, out empty_metadata_t normal_meta, inout headers_t headers, in local_metadata_t local_metadata, in psa_ingress_output_metadata_t istd) {
    apply {
        packet.emit(headers.ethernet);
        packet.emit(headers.ipv4);
        packet.emit(headers.udp);
    }
}

control ingress(inout headers_t headers, inout local_metadata_t local_metadata1, in psa_ingress_input_metadata_t istd, inout psa_ingress_output_metadata_t ostd) {
    action drop() {
        ostd.drop = true;
    }
    action ipv4_forward(PortId_t port) {
        ostd.egress_port = (PortId_t)port;
        headers.ipv4.ttl = headers.ipv4.ttl - 1;
    }
    table ipv4_exact {
        key = {
            headers.ipv4.dst_addr: exact;
            headers.ipv4.src_addr: exact;
        }
        actions = {
            ipv4_forward;
            drop;
        }
        size = 1024;
        default_action = drop();
    }
    apply {
        if (headers.ipv4.isValid()) {
            ipv4_exact.apply();
        }
    }
}

control egress(inout headers_t headers, inout local_metadata_t local_metadata, in psa_egress_input_metadata_t istd, inout psa_egress_output_metadata_t ostd) {
    apply {
    }
}

parser egress_parser(packet_in buffer, out headers_t headers, inout local_metadata_t local_metadata, in psa_egress_parser_input_metadata_t istd, in empty_metadata_t normal_meta, in empty_metadata_t clone_i2e_meta, in empty_metadata_t clone_e2e_meta) {
    state start {
        transition accept;
    }
}

control egress_deparser(packet_out packet, out empty_metadata_t clone_e2e_meta, out empty_metadata_t recirculate_meta, inout headers_t headers, in local_metadata_t local_metadata, in psa_egress_output_metadata_t istd, in psa_egress_deparser_input_metadata_t edstd) {
    apply {
    }
}

IngressPipeline(packet_parser(), ingress(), packet_deparser()) ingress_pipe;

EgressPipeline(egress_parser(), egress(), egress_deparser()) egress_pipe;

PSA_Switch(ingress_pipe, PacketReplicationEngine(), egress_pipe, BufferingQueueingEngine()) main;

