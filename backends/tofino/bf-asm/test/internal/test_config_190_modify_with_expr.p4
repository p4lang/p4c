
/* Sample P4 program */
header_type pkt_t {
    fields {
        srcAddr : 32;
        dstAddr : 32;
        protocol : 8;
        srcPort : 16;
        dstPort : 16;
        
        a : 32;
        b : 32;
        c : 32;
        d : 32;

        e : 16;
        f : 16;
        g : 16;
        h : 16;
    }
}

parser start {
    return parse_ethernet;
}

header pkt_t pkt;

parser parse_ethernet {
    extract(pkt);
    return ingress;
}


action action_0(param0, param1, param2){
    modify_field(pkt.a, pkt.b + 2);
    modify_field(pkt.b, 3 - pkt.b);
    modify_field(pkt.c, pkt.c >> 3);
    modify_field(pkt.d, pkt.d << 7);

    modify_field(pkt.e, param0 | pkt.e);
    modify_field(pkt.f, pkt.f & param1);
    modify_field(pkt.g, pkt.g ^ 0xfff);
    modify_field(pkt.h, ~param2);


}

action nop(){
}

table table_0 {
    reads {                                                                     
        pkt.srcPort   : exact;                                                  
        pkt.dstPort   : ternary;
    }                                                                           
    actions {                                                                   
        nop;                                                                    
        action_0;
    }                                                                           
    size : 4096;
}     


/* Main control flow */

control ingress {
      apply(table_0);
}
