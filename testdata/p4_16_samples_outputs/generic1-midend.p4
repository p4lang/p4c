extern Generic<T> {
    Generic(T sz);
    R get<R>();
    R get1<R, S>(in S value, in R data);
}

extern void f<T>(in T arg);
control caller() {
    bit<32> a_0;
    bit<32> b_0;
    @name("cinst.x") Generic<bit<8>>(8w9) cinst_x;
    action act() {
        a_0 = cinst_x.get<bit<32>>();
        b_0 = (bit<32>)(cinst_x.get1<bit<5>, bit<10>>(10w0, 5w0));
        f<bit<32>>(b_0);
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

control s();
package p(s parg);
p(caller()) main;
