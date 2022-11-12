struct S {
    bit<32> a;
    bit<32> b;
}

control proto();
package top(proto _p);
control c() {
    apply {
        bool b1 = { 1, 2 } == { 1, 2 };
        bool b2 = {a = 32w1,b = 32w2} == {a = 32w1,b = 32w2};
        bool b2_ = {a = 1,b = 2} == {a = 1,b = 2};
    }
}

top(c()) main;
