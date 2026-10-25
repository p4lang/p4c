error {
    IPv4FragmentsNotSupported,
    IPv4OptionsNotSupported,
    IPv4IncorrectVersion
}
#include <core.p4>

header Ethernet {
    bit<16> etherType;
}

header IPv4 {
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

header IPv6 {
}

struct Parsed_packet {
    Ethernet ethernet;
    IPv4     ip_ipv4;
    IPv6     ip_ipv6;
}

