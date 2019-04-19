extern crc_poly<O> {
    crc_poly(O poly);
    O hash<T>(in T data);
}

header h1_t {
    bit<32> f1;
    bit<32> f2;
    bit<32> f3;
}

struct hdrs {
    h1_t    h1;
    bit<16> crc;
}

