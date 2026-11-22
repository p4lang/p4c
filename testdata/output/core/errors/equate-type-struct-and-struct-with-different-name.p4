parser Parser<H, M>(out H parsedHdr, inout M meta);
control Ingress<H, M>(inout H hdr, inout M meta);
package V1Switch<H, M>(Parser<H, M> p, Ingress<H, M> ig);
struct Headers {
}

struct Meta {
}

parser p(out Headers h, inout Meta m) {
    state start {
    }
}

control ingress(inout Meta m, inout Headers h) {
    apply {
    }
}

V1Switch(p(), ingress()) main;
