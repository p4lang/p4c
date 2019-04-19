void foo<T>(in T x) {
}
void bar() {
    foo<bool>(true);
}
void main() {
    foo<bit<8>>(8w0);
}
