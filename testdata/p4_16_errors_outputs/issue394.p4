extern C {
    bool get();
}

control X(out bool b) {
    C c;
    apply {
        b = c.get();
    }
}

control Z(out bool a);
package top(Z z);
top(X()) main;

