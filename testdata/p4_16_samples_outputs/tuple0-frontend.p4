extern void f(in tuple<bit<32>, bool> data);
control proto();
package top(proto _p);
control c() {
    tuple<bit<32>, bool> x;
    apply {
        x = { 32w10, false };
        f(x);
        f({ 32w20, true });
    }
}

top(c()) main;

