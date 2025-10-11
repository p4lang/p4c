enum bit<8> E {
    a = 5
}

bit<16> func() {
    return 16w11 * E.a;
}
