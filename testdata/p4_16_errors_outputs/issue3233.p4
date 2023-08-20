struct s {
    bit<1> t;
    bit<1> t1;
}

s func(bit<1> t, bit<1> t1) {
    return (s)(s)(s){ t, t1 };
}
s func1(s t1) {
    return (s)(s)(s)t1;
}
