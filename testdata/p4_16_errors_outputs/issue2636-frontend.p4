struct S<T> {
    T field;
}

extern S<V> f<V>();
struct S_0 {
    bit<32> field;
}

control c(out bit<32> o) {
    @name("c.s") S_0 s_0;
    apply {
        s_0 = f<bit<32>>();
        o = s_0.field;
    }
}

control _c(out bit<32> o);
package top(_c c);
top(c()) main;

