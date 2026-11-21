control c() {
    @name("c.b") action b() {
    }
    apply {
        b();
    }
}

control proto();
package top(proto p);
top(c()) main;
