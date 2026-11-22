extern CRCPolynomial<T> {
    CRCPolynomial(T coeff);
}

extern Hash<W> {
    Hash(CRCPolynomial<_> poly);
}

control Ingress0() {
    Hash<bit<32>, bit<32>>(poly = CRCPolynomial(coeff = 16w263)) hash2;
    apply {
    }
}

