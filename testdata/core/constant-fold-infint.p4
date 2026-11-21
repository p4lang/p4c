const bit<4> a = 0b0101;
const int b = (int)a; // 5
const bit<7> c = (bit<7>)b; // 5

const int<4> d = -1;
const int e = (int)d; // -1
const bit<7> f = (bit<7>)e; // 0b1111111 = 127
const int<7> g = (int<7>)e; // 0b1111111 = -1
const int h = (int)g; // -1
