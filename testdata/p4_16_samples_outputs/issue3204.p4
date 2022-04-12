struct h<t> {
    t f;
}

bit<1> func() {
    h<bit<1>> a;
    a.f = 1;
    return a.f;
}
