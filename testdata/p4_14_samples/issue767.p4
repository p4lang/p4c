parser start {
    return ingress;
}

action nop() { }

table exact {
    reads { standard_metadata.egress_spec: exact; }
    actions { nop; }
}

control ingress {
    apply(exact);
}
