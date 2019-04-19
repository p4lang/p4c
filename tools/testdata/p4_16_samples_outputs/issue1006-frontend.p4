extern R<T> {
    R(T init);
}

struct foo {
    bit<8> field1;
}

control c();
package top(c _c);
control c1() {
    @name("c1.reg0") R<tuple<bit<8>>>({ 8w1 }) reg0_0;
    @name("c1.reg1") R<foo>({ 8w1 }) reg1_0;
    apply {
    }
}

top(c1()) main;

