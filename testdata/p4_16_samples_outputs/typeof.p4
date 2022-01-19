const bit<32> b = 0;
typedef typeof(b) T;
const T c = 1;
control C(out bit<32> x);
package top(C _c);
control impl(out bit<32> x) {
    apply {
        x = c;
    }
}

top(impl()) main;

