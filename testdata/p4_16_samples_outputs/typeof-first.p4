const bit<32> b = 32w0;
typedef typeof(32w0) T;
const T c = 32w1;
control C(out bit<32> x);
package top(C _c);
control impl(out bit<32> x) {
    apply {
        x = 32w1;
    }
}

top(impl()) main;

