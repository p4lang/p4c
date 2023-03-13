#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
}

struct Headers {
    ethernet_t eth_hdr;
}

struct Meta {
}

parser p(packet_in pkt, out Headers hdr, inout Meta m, inout standard_metadata_t sm) {
    state start {
        pkt.extract<ethernet_t>(hdr.eth_hdr);
        transition accept;
    }
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_2() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_3() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_4() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_5() {
    }
    @name("ingress.tables.increment") action tables_increment_0() {
        h.eth_hdr.dst_addr = h.eth_hdr.dst_addr + 48w1;
    }
    @name("ingress.tables.increment") action tables_increment_1() {
        h.eth_hdr.dst_addr = h.eth_hdr.dst_addr + 48w1;
    }
    @name("ingress.tables.increment") action tables_increment_2() {
        h.eth_hdr.dst_addr = h.eth_hdr.dst_addr + 48w1;
    }
    @name("ingress.tables.increment") action tables_increment_3() {
        h.eth_hdr.dst_addr = h.eth_hdr.dst_addr + 48w1;
    }
    @name("ingress.tables.increment") action tables_increment_4() {
        h.eth_hdr.dst_addr = h.eth_hdr.dst_addr + 48w1;
    }
    @name("ingress.tables.prefix|simple_table_1") table tables_prefix_simple_table {
        key = {
            h.eth_hdr.eth_type: exact @name("dummy_name");
        }
        actions = {
            NoAction_1();
            tables_increment_0();
        }
        default_action = NoAction_1();
    }
    @name("ingress.tables.@prefix@simple_table_2") table tables__prefix_simple_table {
        key = {
            h.eth_hdr.eth_type: exact @name("dummy_name");
        }
        actions = {
            NoAction_2();
            tables_increment_1();
        }
        default_action = NoAction_2();
    }
    @name("ingress.tables.<>]{]prefix/simple_table_3") table tables______prefix_simple_table {
        key = {
            h.eth_hdr.eth_type: exact @name("dummy_name");
        }
        actions = {
            NoAction_3();
            tables_increment_2();
        }
        default_action = NoAction_3();
    }
    @name("ingress.tables.!@#$%^&*()_+=-prefix^simple_table_1") table tables_______________prefix_simple_table {
        key = {
            h.eth_hdr.eth_type: exact @name("dummy_name");
        }
        actions = {
            NoAction_4();
            tables_increment_3();
        }
        default_action = NoAction_4();
    }
    @name("ingress.tables.prefix◕‿◕😀ツsimple_table_1") table tables_prefix________________simple_table {
        key = {
            h.eth_hdr.eth_type: exact @name("dummy_name");
        }
        actions = {
            NoAction_5();
            tables_increment_4();
        }
        default_action = NoAction_5();
    }
    apply {
        tables_prefix_simple_table.apply();
        tables__prefix_simple_table.apply();
        tables______prefix_simple_table.apply();
        tables_______________prefix_simple_table.apply();
        tables_prefix________________simple_table.apply();
    }
}

control vrfy(inout Headers h, inout Meta m) {
    apply {
    }
}

control update(inout Headers h, inout Meta m) {
    apply {
    }
}

control egress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    apply {
    }
}

control deparser(packet_out pkt, in Headers h) {
    apply {
        pkt.emit<Headers>(h);
    }
}

V1Switch<Headers, Meta>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
