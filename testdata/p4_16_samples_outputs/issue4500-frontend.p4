enum E {
    e0
}

extern void bar(E x);
extern void baz();
control c() {
    apply {
        bar(E.e0);
        if (E.e0 == E.e0) {
            baz();
        }
    }
}

control proto();
package top(proto p);
top(c()) main;
