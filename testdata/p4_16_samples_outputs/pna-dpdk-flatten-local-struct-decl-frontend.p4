#include <core.p4>
#include <dpdk/pna.p4>

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

struct main_metadata_t {
    bit<16> data;
    bit<16> data1;
}

struct tuple0 {
    bit<16> f0;
    bit<16> f1;
    bit<32> f2;
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
    @name("MainControlImpl.tuple_decl") tuple0 tuple_decl_0;
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("MainControlImpl.h") Hash<bit<16>>(PNA_HashAlgorithm_t.TOEPLITZ) h_0;
    @name("MainControlImpl.a1") action a1() {
        user_meta.data1 = tuple_decl_0.f0;
        user_meta.data = h_0.get_hash<ethernet_t>(hdr.ethernet);
    }
    @name("MainControlImpl.tbl") table tbl_0 {
        key = {
            hdr.ethernet.srcAddr: exact @name("hdr.ethernet.srcAddr");
        }
        actions = {
            NoAction_1();
            a1();
        }
        default_action = NoAction_1();
    }
    apply {
        tbl_0.apply();
    }
}

control MainDeparserImpl(packet_out pkt, in headers_t hdr, in main_metadata_t user_meta, in pna_main_output_metadata_t ostd) {
    apply {
        pkt.emit<ethernet_t>(hdr.ethernet);
        pkt.emit<ipv4_t>(hdr.ipv4);
    }
}

PNA_NIC<headers_t, main_metadata_t, headers_t, main_metadata_t>(MainParserImpl(), PreControlImpl(), MainControlImpl(), MainDeparserImpl()) main;
