struct S {
    bit<32> f;
    bool    s;
}

control proto();
package top(proto _p);
control c() {
    tuple<bit<32>, bool> x_0;
    tuple<bit<32>, bool> y_0;
    S z_0;
    apply {
        x_0 = { 32w10, false };
        y_0 = x_0;
        z_0 = y_0;
    }
}

top(c()) main;
