extern void f(in tuple<bit<32>, bool> data);
control proto();
package top(proto _p);
control c() {
    tuple<bit<32>, bool> x_0;
    apply {
        x_0 = { 32w10, false };
        f(x_0);
        f({ 32w20, true });
    }
}

top(c()) main;

