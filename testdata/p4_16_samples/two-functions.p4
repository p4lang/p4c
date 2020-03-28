bit<8> test1(inout bit<8> x) {
    return x;
}

bit<8> test2(inout bit<8> x) {
    return x;
}

control c(inout bit<8> a) {
    apply {
        test1(a);
        test2(a);
    }
}

control E(inout bit<8> t);
package top(E e);

top(c()) main;
