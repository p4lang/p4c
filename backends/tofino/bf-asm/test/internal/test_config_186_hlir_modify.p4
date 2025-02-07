
header_type ethernet_t {
    fields {
        dstAddr : 48;
        srcAddr : 48;
        etherType : 16;
        blah0 : 16;
        blah1 : 8;
        blah2 : 8;
    }
}

header_type meta_t {
     fields {
         a : 16;
         b : 16;
         c : 16;
         d : 16;

         e : 32;
         f : 32;
         g : 32;
         h : 32;

         i : 8;
         j : 8;
         k : 8;
         l : 8;
     }
}


header ethernet_t ethernet;
metadata meta_t meta;


parser start {
    return parse_ethernet;
}


parser parse_ethernet {
    extract(ethernet);
    return ingress;
}

action action_0(blah1, blah2, blah3){
    modify_field(meta.a, blah1);
    modify_field(meta.b, blah2);
    modify_field(meta.c, blah3);
}

action action_1(){
    modify_field(meta.e, 7);
    modify_field(meta.f, 8);
    modify_field(meta.g, 2097151);  /* 2**21 - 1 */
    modify_field(meta.h, -1);
}

action do_nothing(){}


table table_0 {
    reads {
        ethernet.dstAddr: exact;
    }
    actions {
        action_0;
        do_nothing;
    }
    size : 1024;
}

@pragma immediate 0
table table_1 {
    reads {
        ethernet.dstAddr: exact;
    }
    actions {
        action_1;
        do_nothing;
    }
    size : 1024;
}

control ingress { 
    apply(table_0);
    apply(table_1);
}
