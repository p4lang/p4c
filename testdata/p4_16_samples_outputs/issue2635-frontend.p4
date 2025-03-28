struct S<T, N> {
    T       field;
    bit<32> otherField;
}

struct S_bit32_bit32 {
    bit<32> field;
    bit<32> otherField;
}

control _c<T>(out T t);
package top<T>(_c<T> c);
control c(out S_bit32_bit32 t) {
    apply {
        t = (S_bit32_bit32){field = 32w0,otherField = 32w0};
    }
}

top<S_bit32_bit32>(c()) main;
