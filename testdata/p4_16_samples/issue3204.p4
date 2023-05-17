struct h<t> {
    t f;
}

bit func() {
    h<bit> a;
    a.f = 1;
    return a.f;
}
