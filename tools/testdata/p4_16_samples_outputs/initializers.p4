#include <core.p4>

extern Fake {
    Fake();
    void call(in bit<32> data);
}

parser P() {
    bit<32> x = 0;
    Fake() fake;
    state start {
        fake.call(x);
        transition accept;
    }
}

control C() {
    bit<32> x = 0;
    bit<32> y = x + 1;
    Fake() fake;
    apply {
        fake.call(y);
    }
}

parser SimpleParser();
control SimpleControl();
package top(SimpleParser prs, SimpleControl ctrl);
top(P(), C()) main;

