control c() {
    @name("c.a") action a() {
    }
    apply {
        a();
    }
}

control proto();
package top(proto p);
top(c()) main;
