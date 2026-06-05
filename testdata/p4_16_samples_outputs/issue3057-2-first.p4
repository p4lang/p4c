struct S {
    bit<32> a;
    bit<32> b;
}

control proto();
package top(proto _p);
control c() {
    apply {
        bool b5 = true;
        bool b5_ = true;
    }
}

top(c()) main;
