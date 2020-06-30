control C() {
    apply {
    }
}

void f<T>(T t) {
}
control D() {
    C() c;
    apply {
        f(c);
    }
}

