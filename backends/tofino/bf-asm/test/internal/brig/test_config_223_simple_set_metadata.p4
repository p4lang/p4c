// #include "tofino/intrinsic_metadata.p4"

header_type zero_byte_t {
    fields {
        a : 8;
    }
}

header_type one_byte_t {
    fields {
        a : 8;
    }
}

header_type meta_t {
   fields {
        a : 32;
        b : 8;
        c : 5;
        d : 3;
   }
}

header zero_byte_t zero;
header one_byte_t one;
metadata meta_t meta;

parser start {
   return zerob;
}

parser zerob {
   extract(zero);
   return select(zero.a) {
       0 : oneb;
       default : twob;
  }
}


parser oneb {
   extract(one);
   set_metadata(meta.b, latest.a);
   set_metadata(meta.a, 0x7);
   set_metadata(meta.c, 1);
   set_metadata(meta.d, 2);  //current(8, 1));
   return ingress;
}

parser twob {
   extract(one);
   set_metadata(meta.c, 2);
   return ingress;
}


action do_nothing(){}

action action_0(p){
    add(one.a, one.a, 1);
    add(meta.a, meta.a, p);
}


table table_i0 {
    reads {
        one.a : ternary;
        meta.a : exact;
        meta.b : exact;
        meta.c : exact;
        meta.d : exact;
    }
    actions {
        do_nothing;
        action_0;
    }
    size : 512;
}

control ingress {
    apply(table_i0);
}
