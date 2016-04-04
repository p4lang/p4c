/*
 * Simple stat program. 
 * Statically mapped, that should be optimized away by the compiler
 * because there isn't a count primitive in any of the actions
 * 
 */

header_type ethernet_t {
    fields {
        dstAddr : 48;
    }
}
parser start {
    return parse_ethernet;
}

header ethernet_t ethernet;

parser parse_ethernet {
    extract(ethernet);
    return ingress;
}
action act(idx) {
    modify_field(ethernet.dstAddr, idx);   
}
table tab1 {
    reads {
        ethernet.dstAddr : exact;
    }
    actions {
        act;
    }
  size: 128;
}

counter cnt {
	type: packets;
	direct: tab1;
}

counter cnt2 {
        type: bytes;
        direct: tab1;


}

control ingress {
    apply(tab1);
}
control egress {

}
