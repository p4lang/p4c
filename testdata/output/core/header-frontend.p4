header my_header {
    bit<1> g1;
    bit<2> g2;
    bit<3> g3;
}

typedef bit<3> Field;
header your_header {
    Field field;
}

header_union your_union {
    your_header h1;
}

struct str {
    your_header hdr;
    your_union  unn;
    bit<32>     dt;
}

