#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

typedef bit<48> EthernetAddress;
typedef bit<32> IPv4Address;
header ethernet_t {
    EthernetAddress dst_addr;
    EthernetAddress src_addr;
    bit<16>         ether_type;
}

header ipv4_t {
    bit<4>      version;
    bit<4>      ihl;
    bit<8>      diffserv;
    bit<16>     total_len;
    bit<16>     identification;
    bit<3>      flags;
    bit<13>     frag_offset;
    bit<8>      ttl;
    bit<8>      protocol;
    bit<16>     hdr_checksum;
    IPv4Address src_addr;
    IPv4Address dst_addr;
}

struct headers_t {
    ethernet_t ethernet;
    ipv4_t     ipv4;
}

enum bit<16> direction_t {
    INVALID = 16w0,
    OUTBOUND = 16w1,
    INBOUND = 16w2
}

struct metadata_t {
    bool            dropped;
    direction_t     direction;
    IPv4Address     dst_ip_addr;
    IPv4Address     src_ip_addr;
    EthernetAddress ethernet_addr;
}

parser test_parser(packet_in packet, out headers_t hd, inout metadata_t meta, inout standard_metadata_t standard_meta) {
    state start {
        packet.extract<ethernet_t>(hd.ethernet);
        transition select(hd.ethernet.ether_type) {
            16w0x800: parse_ipv4;
            default: accept;
        }
    }
    state parse_ipv4 {
        packet.extract<ipv4_t>(hd.ipv4);
        transition accept;
    }
}

control test_deparser(packet_out packet, in headers_t hdr) {
    apply {
        packet.emit<ethernet_t>(hdr.ethernet);
        packet.emit<ipv4_t>(hdr.ipv4);
    }
}

control test_verify_checksum(inout headers_t hdr, inout metadata_t meta) {
    apply {
    }
}

control test_compute_checksum(inout headers_t hdr, inout metadata_t meta) {
    apply {
    }
}

control test_ingress(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t standard_metadata) {
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_2() {
    }
    @name("test_ingress.drop_action") action drop_action() {
        mark_to_drop(standard_metadata);
    }
    @name("test_ingress.action3") action action3() {
        meta.dropped = true;
    }
    @name("test_ingress.action2") action action2() {
        meta.dst_ip_addr = hdr.ipv4.dst_addr;
    }
    @name("test_ingress.action1") action action1() {
        meta.src_ip_addr = hdr.ipv4.dst_addr;
    }
    @name("test_ingress.send_to_port") action send_to_port(@name("p") bit<9> p_1) {
        standard_metadata.egress_spec = p_1;
    }
    @name("test_ingress.pre_tbl1") table pre_tbl1_0 {
        key = {
            hdr.ipv4.dst_addr: exact @name("hdr.ipv4.dst_addr");
        }
        actions = {
            action1();
            action2();
            action3();
            @defaut_only send_to_port();
        }
        const default_action = send_to_port(9w2);
    }
    @name("test_ingress.action4") action action4() {
        meta.direction = direction_t.OUTBOUND;
    }
    @name("test_ingress.in_tbl2") table in_tbl2_0 {
        key = {
            hdr.ipv4.protocol: exact @name("hdr.ipv4.protocol");
        }
        actions = {
            action4();
            @defaultonly NoAction_1();
        }
        default_action = NoAction_1();
    }
    @name("test_ingress.action5") action action5(@name("neighbor_mac") EthernetAddress neighbor_mac) {
        meta.ethernet_addr = neighbor_mac;
    }
    @name("test_ingress.post_tbl3") table post_tbl3_0 {
        key = {
            hdr.ipv4.src_addr: exact @name("hdr.ipv4.src_addr");
        }
        actions = {
            action5();
            @defaultonly NoAction_2();
        }
        default_action = NoAction_2();
    }
    apply {
        meta.dropped = false;
        standard_metadata.egress_spec = standard_metadata.ingress_port;
        switch (pre_tbl1_0.apply().action_run) {
            action1: 
            action2: {
                in_tbl2_0.apply();
            }
            default: {
            }
        }
        post_tbl3_0.apply();
        if (meta.dropped) {
            drop_action();
        }
    }
}

control test_egress(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

V1Switch<headers_t, metadata_t>(test_parser(), test_verify_checksum(), test_ingress(), test_egress(), test_compute_checksum(), test_deparser()) main;
