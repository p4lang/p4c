extern Crc16<T> {
}

extern Checksum16 {
    void initialize<T>();
}

extern void f<T>(in T dt);
control q<S>(in S dt) {
    apply {
        f<bit<32>>(32w0);
        f<bit<32>>(32w0);
        f<S>(dt);
    }
}

extern X<D> {
    T f<T>(in D d, in T t);
}

control z<D1, T1>(in X<D1> x, in D1 v, in T1 t) {
    apply {
        T1 r1 = x.f<T1>(v, t);
        T1 r2 = x.f<T1>(v, t);
    }
}

