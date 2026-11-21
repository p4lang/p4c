const bit<8> x = 10;
struct S {
    bit<8> s;
}

action a(in S w, out bit<8> z) {
    z = x + w.s;
}
