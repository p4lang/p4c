header_type ethernet_t {
    fields {
        dstAddr   : 48;
        srcAddr   : 48;
        ethertype : 16;
    }
}

header ethernet_t ethernet;

parser start {
    extract(ethernet);
    return ingress;
}

action do_b() { no_op(); }
action do_d() { no_op(); }
action do_e() { no_op(); }

action nop()  { no_op(); }

table A {
    reads {
        ethernet.dstAddr : exact; 
    }
    actions {
        do_b;
        do_d;
        do_e;
    }
}

table B { actions { nop; }}
table C { actions { nop; }}
table D { actions { nop; }}
table E { actions { nop; }}
table F { actions { nop; }}

control ingress {
    apply(A) {
        do_b {
            apply(B);
            apply(C);
            }
        do_d {
            apply(D);
            apply(C);
            }
        do_e {
            apply(E);
            }
    }
    apply(F);
}

control egress {
}
