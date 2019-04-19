control c(inout bit<8> a) {
    bit<8> b_0;
    apply {
        b_0 = 8w3;
        a = a / b_0;
    }
}

control proto(inout bit<8> _a);
package top(proto _p);
top(c()) main;

