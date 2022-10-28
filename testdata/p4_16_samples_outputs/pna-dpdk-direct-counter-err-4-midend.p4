#include <core.p4>
#include <pna.p4>

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
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
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("MainControlImpl.per_prefix_pkt_byte_count") DirectCounter<bit<80>>(PNA_CounterType_t.PACKETS_AND_BYTES) per_prefix_pkt_byte_count_0;
    @name("MainControlImpl.per_prefix_pkt_byte_count1") DirectCounter<bit<80>>(PNA_CounterType_t.PACKETS_AND_BYTES) per_prefix_pkt_byte_count1_0;
    @name("MainControlImpl.next_hop") action next_hop(@name("oport") bit<32> oport) {
        per_prefix_pkt_byte_count_0.count();
        per_prefix_pkt_byte_count1_0.count();
        send_to_port(oport);
    }
    @name("MainControlImpl.next_hop") action next_hop_1(@name("oport") bit<32> oport_1) {
        per_prefix_pkt_byte_count_0.count();
        per_prefix_pkt_byte_count1_0.count();
        send_to_port(oport_1);
    }
    @name("MainControlImpl.default_route_drop") action default_route_drop() {
        per_prefix_pkt_byte_count1_0.count();
        drop_packet();
    }
    @name("MainControlImpl.default_route_drop") action default_route_drop_1() {
        per_prefix_pkt_byte_count1_0.count();
        drop_packet();
    }
    @name("MainControlImpl.ipv4_da_lpm1") table ipv4_da_lpm1_0 {
        key = {
            hdr.ipv4.dstAddr: exact @name("hdr.ipv4.dstAddr") ;
        }
        actions = {
            next_hop();
            default_route_drop();
        }
        default_action = default_route_drop();
        pna_direct_counter = per_prefix_pkt_byte_count1_0;
    }
    @name("MainControlImpl.ipv4_da_lpm") table ipv4_da_lpm_0 {
        key = {
            hdr.ipv4.dstAddr: exact @name("hdr.ipv4.dstAddr") ;
        }
        actions = {
            next_hop_1();
            default_route_drop_1();
            @defaultonly NoAction_1();
        }
        pna_direct_counter = per_prefix_pkt_byte_count_0;
        default_action = NoAction_1();
    }
    apply {
        if (hdr.ipv4.isValid()) {
            ipv4_da_lpm_0.apply();
            ipv4_da_lpm1_0.apply();
        }
    }
}

control MainDeparserImpl(packet_out pkt, in headers_t hdr, in main_metadata_t user_meta, in pna_main_output_metadata_t ostd) {
    @hidden action pnadpdkdirectcountererr4l150() {
        pkt.emit<ethernet_t>(hdr.ethernet);
        pkt.emit<ipv4_t>(hdr.ipv4);
    }
    @hidden table tbl_pnadpdkdirectcountererr4l150 {
        actions = {
            pnadpdkdirectcountererr4l150();
        }
        const default_action = pnadpdkdirectcountererr4l150();
    }
    apply {
        tbl_pnadpdkdirectcountererr4l150.apply();
    }
}

PNA_NIC<headers_t, main_metadata_t, headers_t, main_metadata_t>(MainParserImpl(), PreControlImpl(), MainControlImpl(), MainDeparserImpl()) main;

