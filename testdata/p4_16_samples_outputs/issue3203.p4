struct h<t> {
    t f;
}

bit<1> func(h<bit<1>> a) {
    return a.f;
}
