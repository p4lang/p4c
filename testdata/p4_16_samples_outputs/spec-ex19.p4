#include <core.p4>

header Mpls_h {
    bit<20> label;
    bit<3>  tc;
    bit<1>  bos;
    bit<8>  ttl;
}

control p() {
    apply {
        Mpls_h[10] mpls_vec;
    }
}

header Tcp_option_end_h {
    bit<8> kind;
}

header Tcp_option_nop_h {
    bit<8> kind;
}

header Tcp_option_ss_h {
    bit<8>  kind;
    bit<32> maxSegmentSize;
}

header Tcp_option_s_h {
    bit<8>  kind;
    bit<24> scale;
}

header Tcp_option_sack_h {
    bit<8>      kind;
    bit<8>      length;
    varbit<256> sack;
}

header_union Tcp_option_h {
    Tcp_option_end_h  end;
    Tcp_option_nop_h  nop;
    Tcp_option_ss_h   ss;
    Tcp_option_s_h    s;
    Tcp_option_sack_h sack;
}

typedef Tcp_option_h[10] Tcp_option_stack;
struct Tcp_option_sack_top {
    bit<8> kind;
    bit<8> length;
}

parser Tcp_option_parser(packet_in b, out Tcp_option_stack vec) {
    state start {
        transition select(b.lookahead<bit<8>>()) {
            8w0x0: parse_tcp_option_end;
            8w0x1: parse_tcp_option_nop;
            8w0x2: parse_tcp_option_ss;
            8w0x3: parse_tcp_option_s;
            8w0x5: parse_tcp_option_sack;
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
        b.extract(vec.next.sack, (bit<32>)(8 * (b.lookahead<Tcp_option_sack_top>()).length - 16));
        transition start;
    }
}

