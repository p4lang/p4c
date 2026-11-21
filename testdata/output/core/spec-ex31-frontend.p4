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

