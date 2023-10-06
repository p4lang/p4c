extern CRCPolynomial<T> {
    CRCPolynomial(T coeff);
}

control IngressT();
package Switch(IngressT i);
control Ingress() {
    @name("Ingress.poly") CRCPolynomial<bit<8>>(coeff = 8w32) poly_0;
    apply {
    }
}

Switch(Ingress()) main;
