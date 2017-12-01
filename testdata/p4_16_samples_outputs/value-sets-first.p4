#include <core.p4>

header Ethernet_h {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

struct Parsed_packet {
    Ethernet_h ethernet;
}

extern ValueSet {
    ValueSet(bit<32> size);
    bit<8> index(in bit<16> proto);
}

parser TopParser(packet_in b, out Parsed_packet p) {
    ValueSet(32w5) ethtype_kinds;
    state start {
        b.extract<Ethernet_h>(p.ethernet);
        transition select(p.ethernet.etherType) {
            16w0x800: parse_ipv4;
            16w0x806: parse_arp;
            16w0x86dd: parse_ipv6;
            default: dispatch_value_sets;
        }
    }
    state dispatch_value_sets {
        bit<8> setIndex = ethtype_kinds.index(p.ethernet.etherType);
        transition select(setIndex) {
            8w1: parse_trill;
            8w2: parse_vlan_tag;
        }
    }
    state parse_ipv4 {
        transition accept;
    }
    state parse_arp {
        transition accept;
    }
    state parse_ipv6 {
        transition accept;
    }
    state parse_trill {
        transition accept;
    }
    state parse_vlan_tag {
        transition accept;
    }
}

parser proto<T>(packet_in p, out T h);
package top<T>(proto<T> _p);
top<Parsed_packet>(TopParser()) main;

