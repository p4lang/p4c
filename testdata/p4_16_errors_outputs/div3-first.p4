control c(inout bit<8> a) {
    bit<8> b = 8w3;
    apply {
        a = a / b;
    }
}

control proto(inout bit<8> _a);
package top(proto _p);
top(c()) main;

