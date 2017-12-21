struct S {
    bit<32> f;
    bool    s;
}

control proto();
package top(proto _p);
control c() {
    tuple<bit<32>, bool> x;
    apply {
        x = { 32w10, false };
    }
}

top(c()) main;

