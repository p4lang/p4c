void foo<T>(in T x) {}

void bar() {
    foo(true);
}

void main() {
    foo(8w0);
}
