const bit<32> zero = 32w0;
const bit<48> tooLarge = 48w0xbbccddeeff00;
const bit<32> one = 32w1;
const bit<32> max = 32w0xffffffff;
const bit<32> z = 1w1;
struct S {
    bit<32> a;
    bit<32> b;
}

const S v = { 32w3, (bit<32>)z };
const bit<32> two = 32w2;
const int<32> twotwo = (int<32>)two;
const bit<32> twothree = (bit<32>)twotwo;
const bit<6> twofour = (bit<6>)(bit<32>)(int<32>)two;
struct T {
    S a;
    S b;
}

const T zz = { { 32w0, 32w1 }, { 32w2, 32w3 } };
const bit<32> x = 32w0;
const bit<32> x1 = ~32w0;
typedef int<32> int32;
const int32 izero = (int32)32w0;
