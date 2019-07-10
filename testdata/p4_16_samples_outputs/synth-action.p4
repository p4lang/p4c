control c(inout bit<32> x) {
    apply {
        x = 10;
        if (x == 10) {
            x = x + 2;
            x = x - 6;
        } else {
            x = x << 2;
        }
    }
}

control n(inout bit<32> x);
package top(n _n);
top(c()) main;

