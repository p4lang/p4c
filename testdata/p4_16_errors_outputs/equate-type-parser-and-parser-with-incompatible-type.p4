parser Parser<IH>(out IH parsedHeaders);
package Ingress<IH>(Parser<IH> p);
struct H {
}

parser ing_parse(out H hdr) {
    state start {
    }
}

Ingress<int>(ing_parse()) ig1;
