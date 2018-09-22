parser start {
    return ingress;
}

parser Cowles {
    return ingress;
}

@pragma packet_entry
parser start_i2e_mirrored {
    return ingress;
}

@pragma packet_entry
parser start_e2e_mirrored {
    return select(current(0, 32)) {
        default : ingress;
        0xAB00 : Cowles;
    }
}

action nop() { }

table exact {
    reads { standard_metadata.egress_spec: exact; }
    actions { nop; }
}

control ingress {
    apply(exact);
}

