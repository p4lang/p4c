enum bit<4> e {
    a = 4w1,
    b = 4w2,
    c = 4w3
}

void f() {
    bit<8> good1;
    good1 = 8w18;
}
const bit<8> good = 8w18;
const bit<4> bad = 4w3;
const bit<8> bad1 = 8w18;
const bit<4> bad2 = 4w0;
