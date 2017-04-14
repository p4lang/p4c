error {
    InvalidOptions
}
#include <core.p4>

header IPv4_no_options_h {
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

header IPv4_options_h {
    varbit<160> options;
}

header Tcp {
    bit<16> port;
}

struct Parsed_headers {
    IPv4_no_options_h ipv4;
    IPv4_options_h    ipv4options;
    Tcp               tcp;
}

