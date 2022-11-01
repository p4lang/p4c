struct S {
    bit<32> a;
    bit<32> b;
}

control proto();
package top(proto _p);
control c() {
    apply {
        bool b5 = (S){a = 32w1,b = 32w2} == (S){a = 32w1,b = 32w2};
        bool b5_ = (S){a = 32w1,b = 32w2} == (S){a = 32w1,b = 32w2};
    }
}

top(c()) main;
