/*
 * Simple program, just a direct mapped RAM
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
    count(cntDum, idx);  
}
table tab1 {
    reads {
        ethernet.dstAddr : exact;
    }
    actions {
        act;
    }
  size: 160000;
}

counter cntDum {
	type: packets;
	static: tab1;
        instance_count: 70000;
        min_width:64;
}
control ingress {
    apply(tab1);
}
control egress {

}
