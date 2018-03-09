extern hash_function<O> {
    O hash<T>(in T data);
}

extern hash_function<T> crc_poly<T>(T poly);
header h1_t {
    bit<32> f1;
    bit<32> f2;
    bit<32> f3;
}

struct hdrs {
    h1_t    h1;
    bit<16> crc;
}

control test(inout hdrs hdr) {
    apply {
        hdr.crc = (crc_poly<bit<16>>(16w0x801a)).hash<h1_t>(hdr.h1);
    }
}

