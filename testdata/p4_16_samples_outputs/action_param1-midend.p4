control c(inout bit<32> x) {
    @name("arg") bit<32> arg_0;
    @name("a") action a_0() {
        arg_0 = 32w15;
        x = 32w15;
    }
    table tbl_a() {
        actions = {
            a_0();
        }
        const default_action = a_0();
    }
    apply {
        tbl_a.apply();
    }
}

control proto(inout bit<32> arg);
package top(proto p);
top(c()) main;
