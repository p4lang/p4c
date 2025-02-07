#include "tofino/intrinsic_metadata.p4"

/* Sample P4 program */
header_type ethernet_t {
    fields {
        dstAddr : 48;
        srcAddr : 48;
        etherType : 16;
    }
}

header_type blah_t {
    fields {
        a : 32;
        b : 32;
        c : 32;
        d : 32;
        e : 16;
        f : 16;
        g : 16;
        h : 16;
        i : 8;
        j : 8;
        k : 8;
        l : 8;
    }
}


header_type ipv4_t {
    fields {
        version : 4;
        ihl : 4;
        diffserv : 8;
        totalLen : 16;
        identification : 16;
        flags : 3;
        fragOffset : 13;
        ttl : 8;
        protocol : 8;
        hdrChecksum : 16;
        srcAddr : 32;
        dstAddr: 32;
    }
}

parser start {
    return parse_ethernet;
}

header ethernet_t ethernet;

parser parse_ethernet {
    extract(ethernet);
    return select(ethernet.etherType) {
        0x800 : parse_ipv4;
        default: ingress;
    }
}

header ipv4_t ipv4;

parser parse_ipv4 {
    extract(ipv4);
    return parse_blah;
}

header blah_t blah;

parser parse_blah {
     extract(blah);
     return ingress;
}



action action_0(param0){
    bit_xor(blah.a, blah.b, param0);
}

action do_nothing(){
    no_op();
}

table table_0 {
   reads {
       ipv4.dstAddr : exact ;
   }
   actions {
       action_0;
       do_nothing;
   }
}


/* Main control flow */

control ingress {
    apply(table_0);
}
