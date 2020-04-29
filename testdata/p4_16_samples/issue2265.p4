T f<T>(T data) {
    T tmp;
    tmp = data;
    return tmp;
}

control c<T>(inout T data) {
    apply {
        T tmp;
        data = tmp;
    }
}
