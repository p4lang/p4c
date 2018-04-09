extern void f<T>(in T data);
control proto();
package top(proto _p);
control c() {
    tuple<bit<32>, bool> x;
    apply {
        x = { 32w10, false };
        f<tuple<bit<32>, bool>>(x);
    }
}

top(c()) main;

