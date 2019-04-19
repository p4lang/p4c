extern void f<T>(in T data);
control proto();
package top(proto _p);
control c() {
    tuple<bit<32>, bool> x = { 10, false };
    apply {
        f(x);
    }
}

top(c()) main;

