#include <core.p4>

header Ipv4_base {
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

header Ipv4_option_NOP {
    bit<8> value;
}

struct Parsed_Packet {
    Ipv4_base          ipv4;
    Ipv4_option_NOP[3] nop;
}

package Switch();
Switch() main;
