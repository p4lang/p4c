parser start {
    return ingress;
}

@pragma packet_entry
parser start_i2e_mirrored {
    return ingress;
}

@pragma packet_entry
parser start_e2e_mirrored {
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
