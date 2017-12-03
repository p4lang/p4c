typedef tuple<bit<32>, bool> pair;
struct S {
    bit<32> f;
    bool    s;
}

control proto();
package top(proto _p);
control c() {
    pair x = { 10, false };
    tuple<bit<32>, bool> y;
    apply {
        y = x;
    }
}

top(c()) main;

