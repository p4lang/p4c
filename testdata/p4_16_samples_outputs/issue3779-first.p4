header h {
}

bool f() {
    return true;
}
control c(out bool b) {
    apply {
        b = f();
    }
}

control C(out bool b);
package top(C _c);
top(c()) main;
