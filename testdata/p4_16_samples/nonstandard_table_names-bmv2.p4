#include <core.p4>
#include <v1model.p4>

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
}

struct Headers {
    ethernet_t eth_hdr;
}

struct Meta {}

parser p(packet_in pkt, out Headers hdr, inout Meta m, inout standard_metadata_t sm) {
    state start {
        transition parse_hdrs;
    }
    state parse_hdrs {
        pkt.extract(hdr.eth_hdr);
        transition accept;
    }
}


control tables(inout Headers h, inout Meta m, inout standard_metadata_t s) {
    action increment() {
        h.eth_hdr.dst_addr = h.eth_hdr.dst_addr + 1;
    }

    @name("prefix|simple_table_1")
    table simple_table_1 {
        key = {
            h.eth_hdr.eth_type: exact @name("dummy_name");
        }
        actions = {
            NoAction;
            increment;
        }
    }
    @name("@prefix@simple_table_2")
    table simple_table_2 {
        key = {
            h.eth_hdr.eth_type: exact @name("dummy_name") ;
        }
        actions = {
            NoAction;
            increment;
        }
    }
    @name("<>]{]prefix/simple_table_3")
    table simple_table_3 {
        key = {
            h.eth_hdr.eth_type: exact @name("dummy_name") ;
        }
        actions = {
            NoAction;
            increment;
        }
    }
    @name("!@#$%^&*()_+=-prefix^simple_table_1")
    table simple_table_4 {
        key = {
            h.eth_hdr.eth_type: exact @name("dummy_name") ;
        }
        actions = {
            NoAction;
            increment;
        }
    }
    @name("prefix◕‿◕😀ツsimple_table_1")
    table simple_table_5 {
        key = {
            h.eth_hdr.eth_type: exact @name("dummy_name") ;
        }
        actions = {
            NoAction;
            increment;
        }
    }

    apply {
        simple_table_1.apply();
        simple_table_2.apply();
        simple_table_3.apply();
        simple_table_4.apply();
        simple_table_5.apply();
    }
}


control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {

    apply {
        tables.apply(h, m, sm);
    }
}

control vrfy(inout Headers h, inout Meta m) { apply {} }

control update(inout Headers h, inout Meta m) { apply {} }

control egress(inout Headers h, inout Meta m, inout standard_metadata_t sm) { apply {} }

control deparser(packet_out pkt, in Headers h) {
    apply {
        pkt.emit(h);
    }
}

V1Switch(p(), vrfy(), ingress(), egress(), update(), deparser()) main;

