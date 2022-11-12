struct S<T, N> {
    T       field;
    bit<32> otherField;
}

struct S_0 {
    bit<32> field;
    bit<32> otherField;
}

const S_0 s = (S_0){field = 32w0,otherField = 32w0};
control _c<T>(out T t);
package top<T>(_c<T> c);
control c(out S_0 t) {
    apply {
        t = (S_0){field = 32w0,otherField = 32w0};
    }
}

top<S_0>(c()) main;
