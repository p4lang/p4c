const bit<4> one = 1;
const bit<4> max = 0xf;
const bit<4> value = max |+| one;
const bit<4> value1 = max |+| 2;

header h{}


// Pre static_assert static assert until static_assert support is added
void f(in h[value == max ? 1 : -1] a){}
void f1(in h[(max |+| max) == max ? 1 : -1] a){}
void f1(in h[(max |+| 0) == max ? 1 : -1] a){}
void f1(in h[value1 == max ? 1 : -1] a){}


const int<5> ones = 1;
const int<5> maxs = 0xf;
const int<5> values = maxs |+| ones;
const int<5> values1 = maxs |+| 2;
void f(in h[values == maxs ? 1 : -1] a){}
void f1(in h[(maxs |+| maxs) == maxs ? 1 : -1] a){}
void f1(in h[(maxs |+| 0) == maxs ? 1 : -1] a){}
void f1(in h[values1 == maxs ? 1 : -1] a){}
