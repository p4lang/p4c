extern void f<T>(in T data);
control proto();
package top(proto _p);
control c() {
    @name("c.x") tuple<bit<32>, bool> x_0;
    apply {
        x_0 = { 32w10, false };
        f<tuple<bit<32>, bool>>(x_0);
    }
}

top(c()) main;
