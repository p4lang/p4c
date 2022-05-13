void x() {}

control c() {
    apply {
        x();
    }
}

control _c();
package top(_c _c);

top(c()) main;
