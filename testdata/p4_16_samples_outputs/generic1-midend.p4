extern Generic<T> {
    Generic(T sz);
    R get<R>();
    R get1<R, S>(in S value, in R data);
}

extern void f<T>(in T arg);
control caller() {
    bit<32> a;
    bit<32> b;
    bit<32> tmp_1;
    bit<5> tmp_2;
    @name("cinst.x") Generic<bit<8>>(8w9) cinst_x_0;
    action act() {
        tmp_1 = cinst_x_0.get<bit<32>>();
        a = tmp_1;
        tmp_2 = cinst_x_0.get1<bit<5>, bit<10>>(10w0, 5w0);
        b = (bit<32>)tmp_2;
        f<bit<32>>(b);
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
