
action test(inout bit<16> a, inout bit<2> b) {
    b = (a << 3)[1:0];
}
