struct S<T> {
    T t;
}

extern E {
    E(list<S<bit<32>>> data);
}

control c() {
    E((list<S<varbit<32>>>){ { 10 } }) e;
    apply {
    }
}

