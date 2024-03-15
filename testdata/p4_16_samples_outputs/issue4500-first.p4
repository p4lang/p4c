enum E {
    e0
}

extern void bar(E x);
extern void baz();
void foo(E y) {
    bar(y);
    if (y == E.e0) {
        baz();
    }
}
control c() {
    apply {
        foo(E.e0);
    }
}

control proto();
package top(proto p);
top(c()) main;
