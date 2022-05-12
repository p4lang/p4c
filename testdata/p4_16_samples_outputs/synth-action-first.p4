control c(inout bit<32> x) {
    apply {
        x = 32w10;
        if (x == 32w10) {
            x = x + 32w2;
            x = x - 32w6;
        } else {
            x = x << 2;
        }
    }
}

control n(inout bit<32> x);
package top(n _n);
top(c()) main;

