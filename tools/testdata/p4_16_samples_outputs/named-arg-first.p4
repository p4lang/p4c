extern void f(in bit<16> x, in bool y);
control c() {
    apply {
        bit<16> xv = 16w0;
        bool b = true;
        f(y = b, x = xv);
    }
}

control empty();
package top(empty _e);
top(c()) main;

