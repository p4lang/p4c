#include <core.p4>

header IPv4_option_NOP {
    bit<8> value;
}

struct Parsed_Packet {
    IPv4_option_NOP[3] nop;
}

package Switch();
Switch() main;

