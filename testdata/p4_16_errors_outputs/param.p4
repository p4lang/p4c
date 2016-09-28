extern E {
    E();
    void call();
}

control c() {
    action a(in E e) {
        e.call();
    }
    E() einst;
    apply {
        a(einst);
    }
}

control none();
package top(none n);
top(c()) main;
