control c(inout bit<8> a) {
    bit<8> b_0;
    bit<8> tmp;
    apply {
        b_0 = 8w3;
        tmp = a / b_0;
        a = tmp;
    }
}

control proto(inout bit<8> _a);
package top(proto _p);
top(c()) main;
