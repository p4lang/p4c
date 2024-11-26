#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header h0_t {
    bit<32> f0;
    bit<32> f1;
}

struct struct_data_t {
    bit<3> f0;
    bit<5> f1;
    bit<8> f2;
}

struct data_t {
    h0_t          hdr;
    struct_data_t data;
    bit<32>       field;
}

parser SimpleParser(inout data_t d);
package SimpleArch(SimpleParser p);
parser ParserImpl(inout data_t d) {
    state start {
        log_msg("Flattened hierarchical data: {}", { d });
        transition accept;
    }
}

SimpleArch(ParserImpl()) main;
