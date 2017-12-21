control c(inout bit<8> a) {
    bit<8> b;
    apply {
        b = 8w3;
        a = a / b;
    }
}

control proto(inout bit<8> _a);
package top(proto _p);
top(c()) main;

