parser Parser<IH>(out IH parsedHeaders);
package Ingress<IH>(Parser<IH> p);
package Switch<IH>(Ingress<IH> ingress);
struct H {
}

parser ing_parse(out H hdr) {
    state start {
    }
}

Ingress(ing_parse()) ig1;
Switch<tuple<>>(ig1) main;
