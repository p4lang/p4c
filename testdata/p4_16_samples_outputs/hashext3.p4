enum HashAlgorithm_t {
    IDENTITY,
    RANDOM
}

extern HashAlgorithm_t identity_hash(bool msb, bool extend);
extern HashAlgorithm_t random_hash(bool msb, bool extend);
extern Hash<W> {
    Hash(HashAlgorithm_t algo);
    W get<D>(in D data);
}

header h1_t {
    bit<32> f1;
    bit<32> f2;
    bit<32> f3;
}

struct hdrs {
    h1_t    h1;
    bit<16> hash;
}

control test(inout hdrs hdr) {
    apply {
        hdr.hash = Hash<bit<16>>(random_hash(false, false)).get(hdr.h1);
    }
}

