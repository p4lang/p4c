control C(out bit<32> x);
package top(C _c);
control impl(out bit<32> x) {
    apply {
        x = 32w1;
    }
}

top(impl()) main;

