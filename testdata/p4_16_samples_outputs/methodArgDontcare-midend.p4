extern E<T> {
    E();
    void f(in T arg);
}

control c() {
    @name("c.e") E<_>() e_0;
    @hidden action methodArgDontcare9() {
        e_0.f(0);
    }
    @hidden table tbl_methodArgDontcare9 {
        actions = {
            methodArgDontcare9();
        }
        const default_action = methodArgDontcare9();
    }
    apply {
        tbl_methodArgDontcare9.apply();
    }
}

control proto();
package top(proto p);
top(c()) main;
