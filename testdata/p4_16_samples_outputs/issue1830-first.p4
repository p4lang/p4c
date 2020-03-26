void foo_0(in bool x) {
}
void foo_1(in bit<8> x) {
}
void foo<T>(in T x) {
}
void bar() {
    foo_0(true);
}
void main() {
    foo_1(8w0);
}
