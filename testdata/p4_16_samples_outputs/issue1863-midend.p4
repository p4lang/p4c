struct S {
    bit<1> a;
    bit<1> b;
}

control c(out bit<1> b) {
    @hidden action act() {
        b = 1w1;
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    apply {
        tbl_act.apply();
    }
}

control e<T>(out T t);
package top<T>(e<T> e);
top<bit<1>>(c()) main;

