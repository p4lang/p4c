header_type ethernet_t {
    fields {
        dst_addr        : 48; // width in bits
        src_addr        : 48;
        ethertype       : 16;
    }
}

header_type ipv4_t {
    fields {
        version         : 4;
        ihl             : 4;
        diffserv        : 8;
        totalLen        : 16;
        identification  : 16;
        flags           : 3;
        fragOffset      : 13;
        ttl             : 8;
        protocol        : 8;
        hdrChecksum     : 16;
        srcAddr         : 32;
        dstAddr         : 32;
    }
}

header_type tcp_t {
    fields {
        srcPort : 16;
        dstPort : 16;
        seqNo : 32;
        ackNo : 32;
        dataOffset : 4;
        res : 4;
        flags : 8;
        window : 16;
        checksum : 16;
        urgentPtr : 16;
    }
}

header_type udp_t {
    fields {
        srcPort : 16;
        dstPort : 16;
        length_ : 16;
        checksum : 16;
    }
}

header ethernet_t ethernet;
header ipv4_t ipv4;
header tcp_t tcp;
header udp_t udp;

// Start with ethernet always.
parser start {
    return ethernet;
}

parser ethernet {
    extract(ethernet);   // Start with the ethernet header
    return select(ethernet.ethertype) {
        0x800: ipv4;
//        default: ingress;
    }
}

parser ipv4 {
    extract(ipv4);
    return select(latest.fragOffset, latest.protocol) {
        //1 : icmp;
        6 : tcp;
        17 : udp;
        default: ingress;
    }
}

parser tcp {
    extract(tcp);
    return ingress;
}

parser udp {
    extract(udp);
    return ingress;
}

action nop() {}
action set_dip() {}

@pragma proxy_hash_width 24
table exm_proxy_hash {
    reads {
        ipv4.srcAddr  : exact;
        ipv4.dstAddr  : exact;
        ipv4.protocol : exact;
        tcp.srcPort   : exact;
        tcp.dstPort   : exact;
    }
    actions {
        nop;
        set_dip;
    }
    size : 131072;
}

control ingress {
    apply(exm_proxy_hash);
}
