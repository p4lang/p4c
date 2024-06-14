#include <core.p4>

typedef bool bool_t;
typedef bit<8> bit_t;
typedef int<8> int_t;
enum bit_t enum_t {
    A = 1,
    B = 2,
    C = 3,
    D = 4
}

header header_t {
    bool_t f1;
    bit_t  f2;
    int_t  f3;
    enum_t f4;
}

typedef header_t[8] header_stack_t;
struct struct_t {
    bool_t         f1;
    bit_t          f2;
    int_t          f3;
    enum_t         f4;
    header_t       f5;
    header_stack_t f6;
}

struct metadata_t {
    bool_t         bool_f;
    bit_t          bit_f;
    int_t          int_f;
    enum_t         enum_f;
    header_t       header_f;
    header_stack_t header_stack_f;
    struct_t       struct_f;
}

parser TestParser(packet_in pkt, inout metadata_t meta) {
    state start {
        meta.bool_f = pkt.lookahead<bool_t>();
        meta.bit_f = pkt.lookahead<bit_t>();
        meta.int_f = pkt.lookahead<int_t>();
        meta.enum_f = pkt.lookahead<enum_t>();
        meta.header_f = pkt.lookahead<header_t>();
        meta.header_stack_f = pkt.lookahead<header_stack_t>();
        meta.struct_f = pkt.lookahead<struct_t>();
        transition accept;
    }
}

parser Parser<M>(packet_in pkt, inout M meta);
package P<M>(Parser<M> _p);
P(TestParser()) main;
