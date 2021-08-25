#include <core.p4>
#include <pna.p4>

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

typedef bit<48> ByteCounter_t;
typedef bit<80> PacketByteCounter_t;
struct main_metadata_t {
}

struct headers_t {
    ethernet_t ethernet;
    ipv4_t     ipv4;
}

control PreControlImpl(in headers_t hdr, inout main_metadata_t meta, in pna_pre_input_metadata_t istd, inout pna_pre_output_metadata_t ostd) {
    apply {
    }
}

parser MainParserImpl(packet_in pkt, out headers_t hdr, inout main_metadata_t main_meta, in pna_main_parser_input_metadata_t istd) {
    state start {
        pkt.extract<ethernet_t>(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            16w0x800: parse_ipv4;
            default: accept;
        }
    }
    state parse_ipv4 {
        pkt.extract<ipv4_t>(hdr.ipv4);
        transition accept;
    }
}

control MainControlImpl(inout headers_t hdr, inout main_metadata_t user_meta, in pna_main_input_metadata_t istd, inout pna_main_output_metadata_t ostd) {
    @name("MainControlImpl.port_bytes_in") Counter<ByteCounter_t, PortId_t>(32w4, PNA_CounterType_t.BYTES) port_bytes_in_0;
    @name("MainControlImpl.per_prefix_pkt_byte_count") DirectCounter<PacketByteCounter_t>(PNA_CounterType_t.PACKETS_AND_BYTES) per_prefix_pkt_byte_count_0;
    @name("MainControlImpl.next_hop") action next_hop(@name("vport") PortId_t vport) {
        per_prefix_pkt_byte_count_0.count();
        send_to_port(vport);
    }
    @name("MainControlImpl.default_route_drop") action default_route_drop() {
        per_prefix_pkt_byte_count_0.count();
        drop_packet();
    }
    @name("MainControlImpl.ipv4_da_lpm") table ipv4_da_lpm_0 {
        key = {
            hdr.ipv4.dstAddr: lpm @name("hdr.ipv4.dstAddr") ;
        }
        actions = {
            next_hop();
            default_route_drop();
        }
        const default_action = default_route_drop();
        pna_direct_counter = per_prefix_pkt_byte_count_0;
    }
    @hidden action pnaexampletemplate159() {
        port_bytes_in_0.count(istd.input_port);
    }
    @hidden table tbl_pnaexampletemplate159 {
        actions = {
            pnaexampletemplate159();
        }
        const default_action = pnaexampletemplate159();
    }
    apply {
        tbl_pnaexampletemplate159.apply();
        if (hdr.ipv4.isValid()) {
            ipv4_da_lpm_0.apply();
        }
    }
}

control MainDeparserImpl(packet_out pkt, in headers_t hdr, in main_metadata_t user_meta, in pna_main_output_metadata_t ostd) {
    @hidden action pnaexampletemplate174() {
        pkt.emit<ethernet_t>(hdr.ethernet);
        pkt.emit<ipv4_t>(hdr.ipv4);
    }
    @hidden table tbl_pnaexampletemplate174 {
        actions = {
            pnaexampletemplate174();
        }
        const default_action = pnaexampletemplate174();
    }
    apply {
        tbl_pnaexampletemplate174.apply();
    }
}

PNA_NIC<headers_t, main_metadata_t, headers_t, main_metadata_t>(MainParserImpl(), PreControlImpl(), MainControlImpl(), MainDeparserImpl()) main;

