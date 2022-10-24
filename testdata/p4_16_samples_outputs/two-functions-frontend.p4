control c(inout bit<8> a) {
    @name("c.x_0") bit<8> x;
    @name("c.x_1") bit<8> x_2;
    apply {
        x = a;
        a = x;
        x_2 = a;
        a = x_2;
    }
}

control E(inout bit<8> t);
package top(E e);
top(c()) main;
