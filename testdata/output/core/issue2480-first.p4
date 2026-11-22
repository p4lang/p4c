extern CRCPolynomial<T> {
    CRCPolynomial(T coeff);
}

control IngressT();
package Switch(IngressT i);
control Ingress() {
    CRCPolynomial<bit<8>>(coeff = 8w32) poly;
    apply {
    }
}

Switch(Ingress()) main;
