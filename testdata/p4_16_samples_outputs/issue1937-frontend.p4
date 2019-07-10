#include <core.p4>

header h1_t {
    bit<8> f1;
    bit<8> f2;
}

parser parserImpl(out h1_t hdr) {
    bit<8> tmp;
    bit<8> tmp_0;
    state start {
        tmp_0 = hdr.f1;
        transition foo_start;
    }
    state foo_start {
        tmp = tmp_0 >> 2;
        transition start_0;
    }
    state start_0 {
        hdr.f1 = tmp;
        transition foo_start_0;
    }
    state foo_start_0 {
        hdr.f2 = 8w5 >> 2;
        transition start_1;
    }
    state start_1 {
        transition accept;
    }
}

parser p<T>(out T h);
package top<T>(p<T> p);
top<h1_t>(parserImpl()) main;

