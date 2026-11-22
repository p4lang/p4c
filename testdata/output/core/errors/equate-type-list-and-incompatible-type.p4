struct S<T> {
    T t;
}

extern E {
    E(tuple<bool, tuple<>> data);
}

control c() {
    E((list<S<bit<32>>>){ { 10 } }) e;
    apply {
    }
}

