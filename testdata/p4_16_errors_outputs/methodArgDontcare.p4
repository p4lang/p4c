extern E<T> {
    E();
    void f(in T arg);
}

control c() {
    E<_>() e;
    apply {
        e.f(0);
    }
}

control proto();
package top(proto p);
top(c()) main;
