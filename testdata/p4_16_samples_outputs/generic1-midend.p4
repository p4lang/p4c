extern Generic<T> {
    Generic(T sz);
    R get<R>();
    R get1<R, S>(in S value, in R data);
}

extern void f<T>(in T arg);
control caller() {
    bit<5> cinst_tmp_2;
    @name("caller.cinst.x") Generic<bit<8>>(8w9) cinst_x_0;
    @hidden action act() {
        cinst_x_0.get<bit<32>>();
        cinst_tmp_2 = cinst_x_0.get1<bit<5>, bit<10>>(10w0, 5w0);
        f<bit<5>>(cinst_tmp_2);
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

control s();
package p(s parg);
p(caller()) main;

