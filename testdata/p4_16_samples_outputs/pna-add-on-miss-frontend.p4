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

struct main_metadata_t {
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
    @name("MainControlImpl.tmp") bit<32> tmp_0;
    @name("MainControlImpl.next_hop") action next_hop(@name("vport") PortId_t vport) {
        send_to_port(vport);
    }
    @name("MainControlImpl.add_on_miss_action") action add_on_miss_action() {
        tmp_0 = 32w0;
        add_entry<bit<32>>(action_name = "next_hop", action_params = tmp_0, expire_time_profile_id = user_meta.timeout);
    }
    @name("MainControlImpl.ipv4_da") table ipv4_da_0 {
        key = {
            hdr.ipv4.dstAddr: exact @name("hdr.ipv4.dstAddr") ;
        }
        actions = {
            @tableonly next_hop();
            @defaultonly add_on_miss_action();
        }
        add_on_miss = true;
        const default_action = add_on_miss_action();
    }
    @name("MainControlImpl.next_hop2") action next_hop2(@name("vport") PortId_t vport_2, @name("newAddr") bit<32> newAddr) {
        send_to_port(vport_2);
        hdr.ipv4.srcAddr = newAddr;
    }
    @name("MainControlImpl.add_on_miss_action2") action add_on_miss_action2() {
        add_entry<tuple<bit<32>, bit<32>>>(action_name = "next_hop2", action_params = { 32w0, 32w1234 }, expire_time_profile_id = user_meta.timeout);
    }
    @name("MainControlImpl.ipv4_da2") table ipv4_da2_0 {
        key = {
            hdr.ipv4.dstAddr: exact @name("hdr.ipv4.dstAddr") ;
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
            ipv4_da_0.apply();
            ipv4_da2_0.apply();
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

