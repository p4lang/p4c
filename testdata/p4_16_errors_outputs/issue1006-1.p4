extern R<T> {
    R(T init);
}

struct foo {
    bit<8> field1;
}

control c();
package top(c _c);
control c1() {
    R<bit<8>>(16w1) reg2;
    apply {
    }
}

top(c1()) main;

