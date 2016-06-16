parser start {
    return ingress;
}

action _nop() { }

table empty_key {
    actions { _nop; }
}

control ingress {
    apply(empty_key);
}
