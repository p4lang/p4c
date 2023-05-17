#include <core.p4>
#include <v1model.p4>

#define IPV4_ETHTYPE 0x0800
typedef bit<48>  EthernetAddress;
typedef bit<32>  IPv4Address;
typedef bit<128> IPv6Address;
typedef bit<128> IPv4ORv6Address;

header ethernet_t {
    EthernetAddress dst_addr;
    EthernetAddress src_addr;
    bit<16>         ether_type;
}

const bit<16> ETHER_HDR_SIZE=112/8;

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
    INVALID = 0,
    OUTBOUND = 1,
    INBOUND = 2
}

struct metadata_t {
    bool dropped;
    direction_t direction;
    IPv4Address dst_ip_addr;
    IPv4Address src_ip_addr;
    EthernetAddress ethernet_addr;
}

parser test_parser(packet_in packet,
                out headers_t hd,
                inout metadata_t meta,
                inout standard_metadata_t standard_meta)
{
    state start {
        packet.extract(hd.ethernet);
        transition select(hd.ethernet.ether_type) {
            IPV4_ETHTYPE:  parse_ipv4;
            default: accept;
        }
    }

    state parse_ipv4 {
        packet.extract(hd.ipv4);
        transition accept;
    }

}

control test_deparser(packet_out packet,
                   in headers_t hdr)
{
    apply {
	packet.emit(hdr.ethernet);
    packet.emit(hdr.ipv4);
    }
}

control test_verify_checksum(inout headers_t hdr,
                         inout metadata_t meta)
{
    apply { }
}

control test_compute_checksum(inout headers_t hdr,
                          inout metadata_t meta)
{
    apply { }
}

control test_ingress(inout headers_t hdr,
                  inout metadata_t meta,
                  inout standard_metadata_t standard_metadata)
{
    action drop_action() {
        mark_to_drop(standard_metadata);
    }

    action action3() {
        meta.dropped = true;
    }

    action action2() {
        meta.dst_ip_addr = hdr.ipv4.dst_addr;
    }
    action action1() {
        meta.src_ip_addr = hdr.ipv4.dst_addr;
    }

    table pre_tbl1 {
        key = {
            hdr.ipv4.dst_addr : exact;
        }

        actions = {
            action1;
            action2;
            action3;
        }
    }

    action action4() {
        meta.direction = direction_t.OUTBOUND;
    }

    table in_tbl2 {
        key = {
            hdr.ipv4.protocol : exact;
        }

        actions = {
            action4;
        }
    }

    action action5(EthernetAddress neighbor_mac) {
        meta.ethernet_addr = neighbor_mac;
    }

    table post_tbl3 {
        key = {
            hdr.ipv4.src_addr : exact;
        }

        actions = {
            action5;
        }
    }

    apply {

        /* Send packet on same port it arrived (echo) by default */
        standard_metadata.egress_spec = standard_metadata.ingress_port;

        switch (pre_tbl1.apply().action_run) {
            action1:             // Fall-through
            action2: {
                in_tbl2.apply();
            }
        }
        post_tbl3.apply();
        if (meta.dropped) {
            drop_action();
        }
    }
}

control test_egress(inout headers_t hdr,
                 inout metadata_t meta,
                 inout standard_metadata_t standard_metadata)
{
    apply { }
}

V1Switch(test_parser(),
         test_verify_checksum(),
         test_ingress(),
         test_egress(),
         test_compute_checksum(),
         test_deparser()) main;
