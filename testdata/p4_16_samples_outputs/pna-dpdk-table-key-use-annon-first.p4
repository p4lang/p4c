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
typedef bit<32> PacketCounter_t;
typedef bit<80> PacketByteCounter_t;
const bit<32> NUM_PORTS = 32w4;
struct main_metadata_t {
    bit<32>               key;
    ExpireTimeProfileId_t timeout;
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
    action next_hop(PortId_t vport) {
        send_to_port(vport);
    }
    action add_on_miss_action() {
        bit<32> tmp = 32w0;
        add_entry<bit<32>>(action_name = "next_hop", action_params = tmp, expire_time_profile_id = user_meta.timeout);
    }
    table ipv4_da {
        key = {
            hdr.ipv4.dstAddr: exact @name("ipv4_addr_0") ;
        }
        actions = {
            @tableonly next_hop();
            @defaultonly add_on_miss_action();
        }
        add_on_miss = true;
        const default_action = add_on_miss_action();
    }
    action next_hop2(PortId_t vport, bit<32> newAddr) {
        send_to_port(vport);
        hdr.ipv4.srcAddr = newAddr;
    }
    action add_on_miss_action2() {
        add_entry<tuple<bit<32>, bit<32>>>(action_name = "next_hop", action_params = { 32w0, 32w1234 }, expire_time_profile_id = user_meta.timeout);
    }
    table ipv4_da2 {
        key = {
            user_meta.key: exact @name("user_meta.key") ;
        }
        actions = {
            @tableonly next_hop2();
            @defaultonly add_on_miss_action2();
        }
        add_on_miss = true;
        const default_action = add_on_miss_action2();
    }
    apply {
        if (hdr.ipv4.isValid()) {
            ipv4_da.apply();
            ipv4_da2.apply();
        }
    }
}

control MainDeparserImpl(packet_out pkt, in headers_t hdr, in main_metadata_t user_meta, in pna_main_output_metadata_t ostd) {
    apply {
        pkt.emit<ethernet_t>(hdr.ethernet);
        pkt.emit<ipv4_t>(hdr.ipv4);
    }
}

PNA_NIC<headers_t, main_metadata_t, headers_t, main_metadata_t>(MainParserImpl(), PreControlImpl(), MainControlImpl(), MainDeparserImpl()) main;

