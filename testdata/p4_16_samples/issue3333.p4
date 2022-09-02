enum bit<8> myenum {  value = 0 }
parser MyParser1(in  myenum a) {
    state start {
        transition select(a) {
                1 .. 22: state1;
                _: accept;
        }
    }
    state state1 {
        transition accept;
    }
}
