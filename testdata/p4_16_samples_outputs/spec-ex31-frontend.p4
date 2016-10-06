#include <core.p4>

struct EthernetHeader {
    bit<16> etherType;
}

struct IPv4 {
    bit<16> protocol;
}

struct Packet_header {
    EthernetHeader ethernet;
    IPv4           ipv4;
}

