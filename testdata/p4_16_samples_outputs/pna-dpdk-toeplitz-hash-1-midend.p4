#include <core.p4>
#include <dpdk/pna.p4>

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

struct main_metadata_t {
    bit<32> port;
    bit<32> hash;
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

Hash<bit<32>>(PNA_HashAlgorithm_t.CRC32) rss0;
struct tuple_0 {
    bit<32> f0;
    bit<32> f1;
}

control MainControlImpl(inout headers_t hdr, inout main_metadata_t main_meta, in pna_main_input_metadata_t istd, inout pna_main_output_metadata_t ostd) {
    @hidden action pnadpdktoeplitzhash1l94() {
        main_meta.hash = rss0.get_hash<tuple_0>((tuple_0){f0 = hdr.ipv4.srcAddr,f1 = hdr.ipv4.dstAddr});
        main_meta.hash = main_meta.hash & 32w3;
        main_meta.port = main_meta.hash;
    }
    @hidden table tbl_pnadpdktoeplitzhash1l94 {
        actions = {
            pnadpdktoeplitzhash1l94();
        }
        const default_action = pnadpdktoeplitzhash1l94();
    }
    apply {
        tbl_pnadpdktoeplitzhash1l94.apply();
    }
}

control MainDeparserImpl(packet_out pkt, in headers_t hdr, in main_metadata_t user_meta, in pna_main_output_metadata_t ostd) {
    @hidden action pnadpdktoeplitzhash1l107() {
        pkt.emit<ethernet_t>(hdr.ethernet);
        pkt.emit<ipv4_t>(hdr.ipv4);
    }
    @hidden table tbl_pnadpdktoeplitzhash1l107 {
        actions = {
            pnadpdktoeplitzhash1l107();
        }
        const default_action = pnadpdktoeplitzhash1l107();
    }
    apply {
        tbl_pnadpdktoeplitzhash1l107.apply();
    }
}

PNA_NIC<headers_t, main_metadata_t, headers_t, main_metadata_t>(MainParserImpl(), PreControlImpl(), MainControlImpl(), MainDeparserImpl()) main;
