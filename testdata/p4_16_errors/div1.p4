action act() {
    bit<8> a;
    a = a / -1; // not defined for negative numbers
    a = -5 / a;
    a = a % -1; 
    a = -5 % a;
}
