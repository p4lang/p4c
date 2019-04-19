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
            (0, 0 &&& 0): accept;
            (0 &&& 0, 0x44): ipv4_option_NOP;
        }
    }
    state ipv4_option_NOP {
        b.extract(p.nop.next);
        transition start;
    }
}

package Switch();
Switch() main;

