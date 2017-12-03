struct S {
    bit<32> f;
    bool    s;
}

control proto();
package top(proto _p);
struct tuple_0 {
    bit<32> field;
    bool    field_0;
}

control c() {
    apply {
    }
}

top(c()) main;

