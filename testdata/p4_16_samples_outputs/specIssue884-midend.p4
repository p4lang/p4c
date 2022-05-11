enum HashAlgorithm_t {
    IDENTITY,
    RANDOM,
    CRC8,
    CRC16,
    CRC32,
    CRC64
}

extern CRCPolynomial<T> {
    CRCPolynomial(T coeff);
}

extern LCGMatrix<W, H> {
    LCGMatrix(W width, H height, string contents);
}

extern Hash<W> {
    Hash(HashAlgorithm_t algo);
    Hash(CRCPolynomial<_> poly);
    Hash(LCGMatrix<_, _> matrix);
    Hash(string formula);
    W get<D>(in D data);
}

control IngressT<H, M>(inout H hdr, inout M meta);
package Switch<H, M>(IngressT<H, M> ingress);
struct headers_t {
}

struct meta_t {
}

control Ingress(inout headers_t hdr, inout meta_t meta) {
    @name("Ingress.hash1") Hash<bit<32>>(algo = HashAlgorithm_t.CRC32) hash1_0;
    @name("Ingress.tmp") CRCPolynomial<bit<16>>(coeff = 16w0x107) tmp;
    @name("Ingress.hash2") Hash<bit<32>>(poly = tmp) hash2_0;
    @name("Ingress.hamming") LCGMatrix<bit<8>, bit<10>>(8w4, 10w7, "1000100\n         0100011\n         0010110\n         0001001") hamming_0;
    @name("Ingress.hash3") Hash<bit<32>>(matrix = hamming_0) hash3_0;
    @name("Ingress.hash4") Hash<bit<32>>(formula = "magic_formula()") hash4_0;
    apply {
    }
}

Switch<headers_t, meta_t>(Ingress()) main;

