extern Generic<T> {
    Generic(T sz);
    R get<R>();
    R get1<R, S>(in S value, in R data);
}

extern void f<T>(in T arg);
control c_0() {
    bit<5> b_0;
    bit<32> tmp;
    bit<5> tmp_0;
    @name("x") Generic<bit<8>>(8w9) x_0;
    apply {
        tmp = x_0.get<bit<32>>();
        tmp_0 = x_0.get1<bit<5>, bit<10>>(10w0, 5w0);
        b_0 = tmp_0;
        f<bit<5>>(b_0);
    }
}

control caller() {
    @name("cinst") c_0() cinst_0;
    apply {
        cinst_0.apply();
    }
}

control s();
package p(s parg);
p(caller()) main;

