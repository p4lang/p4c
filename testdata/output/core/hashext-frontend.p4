extern hash_function<O, T> {
    O hash(in T data);
}

extern hash_function<O, T> crc_poly<O, T>(O poly);
header h1_t {
    bit<32> f1;
    bit<32> f2;
    bit<32> f3;
}

struct hdrs {
    h1_t    h1;
    bit<16> crc;
}

