#include <core.p4>

header Ethernet_h {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

struct Parsed_packet {
    Ethernet_h ethernet;
}

extern ValueSet<T> {
    ValueSet();
    T members();
}

parser TopParser(packet_in b, out Parsed_packet p) {
    ValueSet<bit<16>>() trill;
    ValueSet<bit<16>>() tpid;
    state start {
        b.extract<Ethernet_h>(p.ethernet);
        transition select(p.ethernet.etherType) {
            16w0x800: parse_ipv4;
            16w0x806: parse_arp;
            16w0x86dd: parse_ipv6;
            trill.members(): parse_trill;
            tpid.members(): parse_vlan_tag;
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
