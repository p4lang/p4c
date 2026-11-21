struct S {
    bit<32> a;
    bit<32> b;
}

control proto();
package top(proto _p);
control c() {
    apply {
        bool b5 = (S){a = 1,b = 2} == {a = 1,b = 2};
        bool b5_ = {a = 1,b = 2} == (S){a = 1,b = 2};
    }
}

top(c()) main;
