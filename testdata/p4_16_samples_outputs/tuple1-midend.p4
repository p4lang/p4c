extern void f<T>(in T data);
control proto();
package top(proto _p);
control c() {
    tuple<bit<32>, bool> x;
    action act() {
        x = { 32w10, false };
        f<tuple<bit<32>, bool>>(x);
    }
    table tbl_act() {
        actions = {
            act();
        }
        const default_action = act();
    }
    apply {
        tbl_act.apply();
    }
}

top(c()) main;
