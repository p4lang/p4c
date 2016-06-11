control c(inout bit<32> x) {
    bit<32> arg_0;
    @name("a") action a() {
        arg_0 = 32w15;
        x = arg_0;
    }
    table tbl_a() {
        actions = {
            a();
        }
        const default_action = a();
    }
    apply {
        tbl_a.apply();
    }
}

control proto(inout bit<32> arg);
package top(proto p);
top(c()) main;
