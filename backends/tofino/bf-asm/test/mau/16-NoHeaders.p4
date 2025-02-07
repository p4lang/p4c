parser start {
    return ingress;
}

action a1() {
}

table t1 {
    actions { 
        a1;
    }
}

control ingress {
    apply(t1);
}

control egress {
}
