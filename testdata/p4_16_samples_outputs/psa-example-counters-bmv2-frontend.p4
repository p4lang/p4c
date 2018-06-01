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
typedef bit<80> PacketByteCounter_t;
struct headers {
    ethernet_t ethernet;
    ipv4_t     ipv4;
}

parser IngressParserImpl(packet_in buffer, out headers parsed_hdr, inout metadata user_meta, in psa_ingress_parser_input_metadata_t istd, in empty_metadata_t resubmit_meta, in empty_metadata_t recirculate_meta) {
    headers parsed_hdr_2;
    metadata user_meta_2;
    state start {
        parsed_hdr_2.ethernet.setInvalid();
        parsed_hdr_2.ipv4.setInvalid();
        user_meta_2 = user_meta;
        transition CommonParser_start;
    }
    state CommonParser_start {
        buffer.extract<ethernet_t>(parsed_hdr_2.ethernet);
        transition select(parsed_hdr_2.ethernet.etherType) {
            16w0x800: CommonParser_parse_ipv4;
            default: start_0;
        }
    }
    state CommonParser_parse_ipv4 {
        buffer.extract<ipv4_t>(parsed_hdr_2.ipv4);
        transition start_0;
    }
    state start_0 {
        parsed_hdr = parsed_hdr_2;
        user_meta = user_meta_2;
        transition accept;
    }
}

parser EgressParserImpl(packet_in buffer, out headers parsed_hdr, inout metadata user_meta, in psa_egress_parser_input_metadata_t istd, in empty_metadata_t normal_meta, in empty_metadata_t clone_i2e_meta, in empty_metadata_t clone_e2e_meta) {
    headers parsed_hdr_3;
    metadata user_meta_3;
    state start {
        parsed_hdr_3.ethernet.setInvalid();
        parsed_hdr_3.ipv4.setInvalid();
        user_meta_3 = user_meta;
        transition CommonParser_start_0;
    }
    state CommonParser_start_0 {
        buffer.extract<ethernet_t>(parsed_hdr_3.ethernet);
        transition select(parsed_hdr_3.ethernet.etherType) {
            16w0x800: CommonParser_parse_ipv4_0;
            default: start_1;
        }
    }
    state CommonParser_parse_ipv4_0 {
        buffer.extract<ipv4_t>(parsed_hdr_3.ipv4);
        transition start_1;
    }
    state start_1 {
        parsed_hdr = parsed_hdr_3;
        user_meta = user_meta_3;
        transition accept;
    }
}

control ingress(inout headers hdr, inout metadata user_meta, in psa_ingress_input_metadata_t istd, inout psa_ingress_output_metadata_t ostd) {
    @name("ingress.port_bytes_in") Counter<ByteCounter_t, PortId_t>(32w512, PSA_CounterType_t.BYTES) port_bytes_in;
    @name("ingress.per_prefix_pkt_byte_count") DirectCounter<PacketByteCounter_t>(PSA_CounterType_t.PACKETS_AND_BYTES) per_prefix_pkt_byte_count;
    @name("ingress.next_hop") action next_hop_0(PortId_t oport) {
        per_prefix_pkt_byte_count.count();
        {
            psa_ingress_output_metadata_t meta_0 = ostd;
            PortId_t egress_port_0 = oport;
            meta_0.drop = false;
            meta_0.multicast_group = 10w0;
            meta_0.egress_port = egress_port_0;
            ostd = meta_0;
        }
    }
    @name("ingress.default_route_drop") action default_route_drop_0() {
        per_prefix_pkt_byte_count.count();
        {
            psa_ingress_output_metadata_t meta_1 = ostd;
            meta_1.drop = true;
            ostd = meta_1;
        }
    }
    @name("ingress.ipv4_da_lpm") table ipv4_da_lpm {
        key = {
            hdr.ipv4.dstAddr: lpm @name("hdr.ipv4.dstAddr") ;
        }
        actions = {
            next_hop_0();
            default_route_drop_0();
        }
        default_action = default_route_drop_0();
        psa_direct_counter = per_prefix_pkt_byte_count;
    }
    apply {
        port_bytes_in.count(istd.ingress_port);
        if (hdr.ipv4.isValid()) 
            ipv4_da_lpm.apply();
    }
}

control egress(inout headers hdr, inout metadata user_meta, in psa_egress_input_metadata_t istd, inout psa_egress_output_metadata_t ostd) {
    @name("egress.port_bytes_out") Counter<ByteCounter_t, PortId_t>(32w512, PSA_CounterType_t.BYTES) port_bytes_out;
    apply {
        port_bytes_out.count(istd.egress_port);
    }
}

control IngressDeparserImpl(packet_out buffer, out empty_metadata_t clone_i2e_meta, out empty_metadata_t resubmit_meta, out empty_metadata_t normal_meta, inout headers hdr, in metadata meta, in psa_ingress_output_metadata_t istd) {
    apply {
        buffer.emit<ethernet_t>(hdr.ethernet);
        buffer.emit<ipv4_t>(hdr.ipv4);
    }
}

control EgressDeparserImpl(packet_out buffer, out empty_metadata_t clone_e2e_meta, out empty_metadata_t recirculate_meta, inout headers hdr, in metadata meta, in psa_egress_output_metadata_t istd, in psa_egress_deparser_input_metadata_t edstd) {
    apply {
        buffer.emit<ethernet_t>(hdr.ethernet);
        buffer.emit<ipv4_t>(hdr.ipv4);
    }
}

IngressPipeline<headers, metadata, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t>(IngressParserImpl(), ingress(), IngressDeparserImpl()) ip;

EgressPipeline<headers, metadata, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t>(EgressParserImpl(), egress(), EgressDeparserImpl()) ep;

PSA_Switch<headers, metadata, headers, metadata, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t>(ip, PacketReplicationEngine(), ep, BufferingQueueingEngine()) main;

