const bit<8> x = 8w10;
struct S {
    bit<8> s;
}

action a(in S w, out bit<8> z) {
    z = 8w10 + w.s;
}
