extern E {
    E();
    void call();
}

control c() {
    E() e1;
    E() e2;
    apply {
        e1 = e2;
    }
}

control none();
package top(none n);
top(c()) main;

