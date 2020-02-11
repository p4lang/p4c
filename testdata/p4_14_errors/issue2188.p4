header_type ethernet_t {
    fields {
        dst_addr        : 48; // width in bits
        src_addr        : 48;
        etherType       : 16;
    }
}
header ethernet_t ethernet;

parser_value_set pvs0;

parser start {
    return parse_ethernet;
}

parser parse_ethernet {
    extract(ethernet);
    return select(latest.etherType) {
        pvs0 mask 0x0ff0 : ingress;       //  <--- this is the most significant line of the example
        default : ingress;
    }
}

control ingress {
}