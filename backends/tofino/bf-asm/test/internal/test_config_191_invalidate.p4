
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


action action_0(){
    invalidate(144);
}

action action_1(){
    invalidate(pkt.a);
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
        action_1;
    }                                                                           
    size : 4096;
}     


/* Main control flow */

control ingress {
      apply(table_0);
}
