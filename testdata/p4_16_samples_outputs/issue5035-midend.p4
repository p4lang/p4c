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
struct tuple_0 {
    bit<32> f0;
    bit<32> f1;
    bit<3>  f2;
    bit<5>  f3;
    bit<8>  f4;
    bit<32> f5;
}

parser ParserImpl(inout data_t d) {
    state start {
        log_msg<tuple_0>("Flattened hierarchical data: (hdr:(f0:{},f1:{}),data:(f0:{},f1:{},f2:{}),field:{})", (tuple_0){f0 = d.hdr.f0,f1 = d.hdr.f1,f2 = d.data.f0,f3 = d.data.f1,f4 = d.data.f2,f5 = d.field});
        transition accept;
    }
}

SimpleArch(ParserImpl()) main;
