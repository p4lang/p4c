header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
}

struct Headers {
    ethernet_t eth_hdr;
}

control c(inout Headers hdr) {
    bit<16> tmp_val = 16w3;
    action do_action() {
        hdr.eth_hdr.eth_type = (tmp_val > 16w2 ? 16w3 : 16w1) + 16w3;
    }
    apply {
        do_action();
        do_action();
    }
}

control proto(inout Headers hdr);
package top(proto _p);
top(c()) main;
