
/* Sample P4 program */
header_type pkt_t {
    fields {
        srcAddr : 32;
        dstAddr : 32;
        protocol : 8;
        srcPort : 16;
        dstPort : 16;

        blah : 16;
        
        a : 32;
        b : 32;
        c : 32;
        d : 32;

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


action set_dip(){
    modify_field(pkt.blah, 8);
}

action nop(){
}

@pragma proxy_hash_width 24                                                     
table exm_proxy_hash {                                                          
    reads {                                                                     
        pkt.srcAddr  : exact;                                                  
        pkt.dstAddr  : exact;                                                  
        pkt.protocol : exact;                                                  
        pkt.srcPort   : exact;                                                  
        pkt.dstPort   : exact;                                                  
    }                                                                           
    actions {                                                                   
        nop;                                                                    
        set_dip;                                                                
    }                                                                           
    size : 400000;                                                              
}     


/* Main control flow */

control ingress {
      apply(exm_proxy_hash);
}
