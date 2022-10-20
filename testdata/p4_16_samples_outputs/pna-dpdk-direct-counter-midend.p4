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

control PreControlImpl(in headers_t hdr, inout main_metadata_t meta, in pna_pre_input_metadata_t istd, inout pna_pre_output_metadata_t ostd) {
    apply {
    }
}

control MainControlImpl(inout headers_t hdr, inout main_metadata_t user_meta, in pna_main_input_metadata_t istd, inout pna_main_output_metadata_t ostd) {
    @name("MainControlImpl.per_prefix_bytes_count") DirectCounter<bit<48>>(PNA_CounterType_t.BYTES) per_prefix_bytes_count_0;
    @name("MainControlImpl.per_prefix_pkt_bytes_count") DirectCounter<bit<80>>(PNA_CounterType_t.PACKETS_AND_BYTES) per_prefix_pkt_bytes_count_0;
    @name("MainControlImpl.per_prefix_pkt_count") DirectCounter<bit<32>>(PNA_CounterType_t.PACKETS) per_prefix_pkt_count_0;
    @name("MainControlImpl.drop") action drop() {
        drop_packet();
    }
    @name("MainControlImpl.drop") action drop_1() {
        drop_packet();
    }
    @name("MainControlImpl.drop") action drop_2() {
        drop_packet();
    }
    @name("MainControlImpl.send") action send(@name("port") bit<32> port) {
        send_to_port(port);
        per_prefix_pkt_count_0.count();
    }
    @name("MainControlImpl.send_bytecount") action send_bytecount(@name("port") bit<32> port_3) {
        send_to_port(port_3);
        per_prefix_bytes_count_0.count(32w1024);
    }
    @name("MainControlImpl.send_pktbytecount") action send_pktbytecount(@name("port") bit<32> port_4) {
        send_to_port(port_4);
        per_prefix_pkt_bytes_count_0.count(32w1024);
    }
    @name("MainControlImpl.ipv4_host") table ipv4_host_0 {
        key = {
            hdr.ipv4.dstAddr: exact @name("hdr.ipv4.dstAddr") ;
        }
        actions = {
            drop();
            send();
        }
        const default_action = drop();
        size = 32w1024;
        pna_direct_counter = per_prefix_pkt_count_0;
    }
    @name("MainControlImpl.ipv4_host_byte_count") table ipv4_host_byte_count_0 {
        key = {
            hdr.ipv4.dstAddr: exact @name("hdr.ipv4.dstAddr") ;
        }
        actions = {
            drop_1();
            send_bytecount();
        }
        const default_action = drop_1();
        size = 32w1024;
        pna_direct_counter = per_prefix_bytes_count_0;
    }
    @name("MainControlImpl.ipv4_host_pkt_byte_count") table ipv4_host_pkt_byte_count_0 {
        key = {
            hdr.ipv4.dstAddr: exact @name("hdr.ipv4.dstAddr") ;
        }
        actions = {
            drop_2();
            send_pktbytecount();
        }
        const default_action = drop_2();
        size = 32w1024;
        pna_direct_counter = per_prefix_pkt_bytes_count_0;
    }
    apply {
        ipv4_host_0.apply();
        ipv4_host_byte_count_0.apply();
        ipv4_host_pkt_byte_count_0.apply();
    }
}

control MainDeparserImpl(packet_out pkt, in headers_t hdr, in main_metadata_t user_meta, in pna_main_output_metadata_t ostd) {
    @hidden action pnadpdkdirectcounter162() {
        pkt.emit<ethernet_t>(hdr.ethernet);
        pkt.emit<ipv4_t>(hdr.ipv4);
    }
    @hidden table tbl_pnadpdkdirectcounter162 {
        actions = {
            pnadpdkdirectcounter162();
        }
        const default_action = pnadpdkdirectcounter162();
    }
    apply {
        tbl_pnadpdkdirectcounter162.apply();
    }
}

PNA_NIC<headers_t, main_metadata_t, headers_t, main_metadata_t>(MainParserImpl(), PreControlImpl(), MainControlImpl(), MainDeparserImpl()) main;

