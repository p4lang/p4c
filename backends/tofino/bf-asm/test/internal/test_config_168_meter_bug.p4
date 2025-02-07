#include "tofino/intrinsic_metadata.p4"
//#include "tofino/stateful_alu_blackbox.p4"


header_type ethernet_t {
    fields {
        dstAddr : 48;
        srcAddr : 48;
        etherType : 16;
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


header ethernet_t ethernet;
header ipv4_t ipv4;

parser start {
    return parse_ethernet;
}


parser parse_ethernet {
    extract(ethernet);
    return select(latest.etherType) {
        0x0800 : parse_ipv4;
        default : ingress;
    }
}

parser parse_ipv4 {
    extract(ipv4);
    return ingress;
}


meter exm_meter2 {                                                              
    type : bytes;                                                               
    direct : table_0;
    result : ipv4.diffserv;                                                     
}  


action nop() {                                                                  
}                                                                               
                                                                                
action action_0(){
    modify_field(ipv4.ttl, 4);
}                                                                               

action action_1(){
    modify_field(ipv4.ttl, 5);
}                                                                               


table table_0 {                                               
    reads {                                                                     
        ipv4.srcAddr : exact;                                                   
        ipv4.dstAddr : exact;
        ipv4.diffserv : exact; /* hack */
    }                                                                           
    actions {                                                                   
        action_0;
        action_1;
        nop;
    }                                                                           
}    

/* Main control flow */

control ingress {
    apply(table_0);
}
