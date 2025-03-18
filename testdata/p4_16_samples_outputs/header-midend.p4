header my_header {
    bit<1> g1;
    bit<2> g2;
    bit<3> g3;
}

header your_header {
    bit<3> field;
}

struct str {
    your_header hdr;
    your_header unn_h1;
    bit<32>     dt;
}

