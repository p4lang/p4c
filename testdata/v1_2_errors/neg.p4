action act() {
    int<8> a;
    int<8> b;
    int<8> c;

    c = a / b;  // not defined for signed values
    c = a % b;
}
