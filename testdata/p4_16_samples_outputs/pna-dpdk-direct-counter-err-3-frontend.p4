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
}

struct headers_t {
    ethernet_t ethernet;
    ipv4_t     ipv4;
}

typedef bit<48> ByteCounter_t;
typedef bit<80> PacketByteCounter_t;
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
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_2() {
    }
    @name("MainControlImpl.per_prefix_bytes_count") DirectCounter<ByteCounter_t>(PNA_CounterType_t.BYTES) per_prefix_bytes_count_0;
    @name("MainControlImpl.per_prefix_pkt_bytes_count") DirectCounter<PacketByteCounter_t>(PNA_CounterType_t.PACKETS_AND_BYTES) per_prefix_pkt_bytes_count_0;
    @name("MainControlImpl.drop") action drop() {
        drop_packet();
    }
    @name("MainControlImpl.drop") action drop_1() {
        drop_packet();
    }
    @name("MainControlImpl.send_bytecount") action send_bytecount(@name("port") PortId_t port) {
        send_to_port(port);
        per_prefix_pkt_bytes_count_0.count(32w1024);
    }
    @name("MainControlImpl.send_pktbytecount") action send_pktbytecount(@name("port") PortId_t port_2) {
        send_to_port(port_2);
        per_prefix_bytes_count_0.count(32w1024);
    }
    @name("MainControlImpl.ipv4_host_byte_count") table ipv4_host_byte_count_0 {
        key = {
            hdr.ipv4.dstAddr: exact @name("hdr.ipv4.dstAddr");
        }
        actions = {
            drop();
            send_bytecount();
            @defaultonly NoAction_1();
        }
        size = 32w1024;
        pna_direct_counter = per_prefix_bytes_count_0;
        default_action = NoAction_1();
    }
    @name("MainControlImpl.ipv4_host_pkt_byte_count") table ipv4_host_pkt_byte_count_0 {
        key = {
            hdr.ipv4.dstAddr: exact @name("hdr.ipv4.dstAddr");
        }
        actions = {
            drop_1();
            send_pktbytecount();
            @defaultonly NoAction_2();
        }
        size = 32w1024;
        pna_direct_counter = per_prefix_pkt_bytes_count_0;
        default_action = NoAction_2();
    }
    apply {
        ipv4_host_byte_count_0.apply();
        ipv4_host_pkt_byte_count_0.apply();
    }
}

control MainDeparserImpl(packet_out pkt, in headers_t hdr, in main_metadata_t user_meta, in pna_main_output_metadata_t ostd) {
    apply {
        pkt.emit<ethernet_t>(hdr.ethernet);
        pkt.emit<ipv4_t>(hdr.ipv4);
    }
}

PNA_NIC<headers_t, main_metadata_t, headers_t, main_metadata_t>(MainParserImpl(), PreControlImpl(), MainControlImpl(), MainDeparserImpl()) main;
