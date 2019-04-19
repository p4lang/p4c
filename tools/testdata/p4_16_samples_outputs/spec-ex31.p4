#include <core.p4>

header EthernetHeader {
    bit<16> etherType;
}

header IPv4 {
    bit<16> protocol;
}

struct Packet_header {
    EthernetHeader ethernet;
    IPv4           ipv4;
}

parser EthernetParser(packet_in b, out EthernetHeader h) {
    state start {
        transition accept;
    }
}

parser GenericParser(packet_in b, out Packet_header p)(bool udpSupport) {
    EthernetParser() ethParser;
    state start {
        ethParser.apply(b, p.ethernet);
        transition select(p.ethernet.etherType) {
            16w0x800: ipv4;
        }
    }
    state ipv4 {
        b.extract(p.ipv4);
        transition select(p.ipv4.protocol) {
            16w6: tryudp;
            16w17: tcp;
        }
    }
    state tryudp {
        transition select(udpSupport) {
            false: reject;
            true: udp;
        }
    }
    state udp {
        transition accept;
    }
    state tcp {
        transition accept;
    }
}

