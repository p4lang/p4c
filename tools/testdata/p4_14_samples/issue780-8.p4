header_type h_t {
    fields { f : 8; }
}

header h_t h;

parser start {
    extract(h);
    return ingress;
}

action nop() { }

table exact {
    reads { h.f : ternary; }
    actions { nop; }
    size : 256;
}

control ingress {
    apply(exact);
}
