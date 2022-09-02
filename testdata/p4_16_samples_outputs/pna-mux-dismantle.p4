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

header tcp_t {
    bit<16> srcPort;
    bit<16> dstPort;
    bit<32> seqNo;
    bit<32> ackNo;
    bit<4>  dataOffset;
    bit<3>  res;
    bit<3>  ecn;
    bit<6>  flags;
    bit<16> window;
    bit<16> checksum;
    bit<16> urgentPtr;
}

struct empty_metadata_t {
}

typedef bit<48> ByteCounter_t;
typedef bit<32> PacketCounter_t;
typedef bit<80> PacketByteCounter_t;
const bit<32> NUM_PORTS = 4;
struct main_metadata_t {
    bit<1>                rng_result1;
    bit<1>                val1;
    bit<1>                val2;
    ExpireTimeProfileId_t timeout;
}

struct headers_t {
    ethernet_t ethernet;
    ipv4_t     ipv4;
    tcp_t      tcp;
}

control PreControlImpl(in headers_t hdr, inout main_metadata_t meta, in pna_pre_input_metadata_t istd, inout pna_pre_output_metadata_t ostd) {
    apply {
    }
}

parser MainParserImpl(packet_in pkt, out headers_t hdr, inout main_metadata_t main_meta, in pna_main_parser_input_metadata_t istd) {
    state start {
        pkt.extract(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            0x800: parse_ipv4;
            default: accept;
        }
    }
    state parse_ipv4 {
        pkt.extract(hdr.ipv4);
        transition accept;
    }
}

bit<1> dummy_func(inout headers_t hdr, bit<16> min1, bit<16> max1, inout main_metadata_t user_meta) {
    return (bit<1>)(min1 <= hdr.tcp.srcPort && hdr.tcp.srcPort <= max1);
}
action do_range_checks_0(inout headers_t hdr, inout main_metadata_t user_meta, bit<16> min1, bit<16> max1) {
    user_meta.rng_result1 = dummy_func(hdr, min1, max1, user_meta);
}
control MainControlImpl(inout headers_t hdr, inout main_metadata_t user_meta, in pna_main_input_metadata_t istd, inout pna_main_output_metadata_t ostd) {
    action next_hop(PortId_t vport) {
        send_to_port(vport);
    }
    action add_on_miss_action() {
        bit<32> tmp = 0;
        add_entry(action_name = "next_hop", action_params = tmp, expire_time_profile_id = user_meta.timeout);
    }
    action do_range_checks_1(bit<16> min1, bit<16> max1) {
        user_meta.rng_result1 = (min1 <= hdr.tcp.srcPort && hdr.tcp.srcPort <= max1 ? (16w50 <= hdr.tcp.srcPort && hdr.tcp.srcPort <= 16w100 ? user_meta.val1 : user_meta.val2) : user_meta.val1);
    }
    table ipv4_da {
        key = {
            hdr.ipv4.dstAddr: exact;
        }
        actions = {
            @tableonly next_hop;
            @defaultonly add_on_miss_action;
        }
        add_on_miss = true;
        const default_action = add_on_miss_action;
    }
    action next_hop2(PortId_t vport, bit<32> newAddr) {
        send_to_port(vport);
        hdr.ipv4.srcAddr = newAddr;
    }
    action add_on_miss_action2() {
        add_entry(action_name = "next_hop", action_params = { 32w0, 32w1234 }, expire_time_profile_id = user_meta.timeout);
    }
    table ipv4_da2 {
        key = {
            hdr.ipv4.dstAddr: exact;
        }
        actions = {
            @tableonly next_hop2;
            @defaultonly add_on_miss_action2;
            do_range_checks_1;
            do_range_checks_0(hdr, user_meta);
        }
        add_on_miss = true;
        const default_action = add_on_miss_action2;
    }
    apply {
        user_meta.rng_result1 = (bit<1>)(100 <= hdr.tcp.srcPort && hdr.tcp.srcPort <= 200);
        if (hdr.ipv4.isValid()) {
            ipv4_da.apply();
            ipv4_da2.apply();
        }
    }
}

control MainDeparserImpl(packet_out pkt, in headers_t hdr, in main_metadata_t user_meta, in pna_main_output_metadata_t ostd) {
    apply {
        pkt.emit(hdr.ethernet);
        pkt.emit(hdr.ipv4);
    }
}

PNA_NIC(MainParserImpl(), PreControlImpl(), MainControlImpl(), MainDeparserImpl()) main;

