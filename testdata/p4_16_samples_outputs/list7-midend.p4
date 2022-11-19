extern E<V> {
    E(V param);
    void run();
}

struct S {
    bit<32> f;
}

control c() {
    @name("c.e") E<S>((S){f = 32w5}) e_0;
    @hidden action list7l14() {
        e_0.run();
    }
    @hidden table tbl_list7l14 {
        actions = {
            list7l14();
        }
        const default_action = list7l14();
    }
    apply {
        tbl_list7l14.apply();
    }
}

control C();
package top(C _c);
top(c()) main;
