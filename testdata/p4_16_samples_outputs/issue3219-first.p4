bit<4> func1() {
    bit<4> t = (bit<4>)(int)1;
    return t;
}
bit<4> func2() {
    bit<4> t = (bit<4>)(int)4w1;
    return t;
}
control c(out bit<4> result) {
    apply {
        result = func1();
    }
}

control _c(out bit<4> r);
package top(_c _c);
top(c()) main;
