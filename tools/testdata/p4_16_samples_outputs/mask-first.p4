#include <core.p4>

header IPv4_option_NOP {
    bit<8> value;
}

struct Parsed_Packet {
    IPv4_option_NOP[3] nop;
}

parser Parser(packet_in b, out Parsed_Packet p) {
    state start {
        transition select(8w0, b.lookahead<bit<8>>()) {
            default: accept;
            (8w0, 8w0 &&& 8w0): accept;
            (8w0 &&& 8w0, 8w0x44): ipv4_option_NOP;
        }
    }
    state ipv4_option_NOP {
        b.extract<IPv4_option_NOP>(p.nop.next);
        transition start;
    }
}

package Switch();
Switch() main;

