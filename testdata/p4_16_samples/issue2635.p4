struct S<T, N> {
    T field;
    bit<32> otherField;
}

const S<bit<32>, bit<32>> s = { 0, 0 };

control _c<T>(out T t);
package top<T>(_c<T> c);

control c(out S<bit<32>, bit<32>> t) {
    apply {
        t = { 0, 0 };
    }
}

top(c()) main;
