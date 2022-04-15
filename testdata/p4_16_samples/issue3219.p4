bit<4> func1() {
    bit<4> t = (int)1;
    return t;
}

bit<4> func2() {
    bit<4> t = (bit<4>)(int)(bit<4>)1;
    return t;
}