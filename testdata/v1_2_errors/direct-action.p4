control c(inout bit<16> y) {
    bit<32> x;
    action a(bit<32> arg) {
        y = (bit<16>)arg;
    }

    apply {
        a(x); // error: not a compile-time constant
    }
}

control proto(inout bit<16> y);
package top(proto _p);

top(c()) main;
