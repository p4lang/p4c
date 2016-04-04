#include "core.p4"

#include "spec-ex09.p4"

struct Tcp_option_sack_top
{
    bit<8> kind;
    bit<8> length;
}
parser Tcp_option_parser(packet_in b, 
                         out Tcp_option_stack vec)
{
    state start {
        transition select(b.lookahead<bit<8>>()) {
            8w0x0 : parse_tcp_option_end;
            8w0x1 : parse_tcp_option_nop;
            8w0x2 : parse_tcp_option_ss;
            8w0x3 : parse_tcp_option_s;
            8w0x5 : parse_tcp_option_sack;
        }
    }
    state parse_tcp_option_end {
        b.extract(vec.next.end);
        transition accept;
    }
    state parse_tcp_option_nop {
         b.extract(vec.next.nop);
         transition start;
    }
    state parse_tcp_option_ss {
         b.extract(vec.next.ss);
         transition start;
    }
    state parse_tcp_option_s {
         b.extract(vec.next.s);
         transition start;
    }
    state parse_tcp_option_sack {
         b.extract(vec.next.sack,
                   (bit<32>)(b.lookahead<Tcp_option_sack_top>().length));
         transition start;
    }
}
