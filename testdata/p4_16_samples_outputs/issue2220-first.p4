#include <core.p4>

enum bit<8> myEnum {
    One = 8w1
}

struct S {
    myEnum val;
}

control c(out S s) {
    apply {
        S s1 = (S){val = 8w0};
        s = s1;
    }
}

control simple<T>(out T t);
package top<T>(simple<T> e);
top<S>(c()) main;

