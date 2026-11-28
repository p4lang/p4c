header H<T> {
    T f;
}

control c() {
    apply {
        H<UndefinedType> h;
    }
}

