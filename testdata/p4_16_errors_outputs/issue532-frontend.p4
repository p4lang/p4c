#include <core.p4>
#include <v1model.p4>

struct s1_t {
    bit<8> f8;
}

struct choices_t {
    s1_t entry0;
    s1_t entry1;
    s1_t entry2;
    s1_t entry3;
}

struct my_meta_t {
    s1_t entry;
}

struct parsed_packet_t {
}

parser parse(packet_in pk, out parsed_packet_t hdr, inout my_meta_t my_metadata, inout standard_metadata_t standard_metadata) {
    state start {
        transition accept;
    }
}

extern s1_t choose_entry(in choices_t choices);
control ingress(inout parsed_packet_t hdr, inout my_meta_t my_meta, inout standard_metadata_t standard_metadata) {
    @name(".NoAction") action NoAction_0() {
    }
    s1_t tmp_0;
    @name("ingress.select_entry") action select_entry_0(choices_t choices) {
        tmp_0 = choose_entry(choices);
        my_meta.entry = tmp_0;
    }
    @name("ingress.t") table t {
        actions = {
            select_entry_0();
            NoAction_0();
        }
        const default_action = NoAction_0();
    }
    apply {
        t.apply();
    }
}

control egress(inout parsed_packet_t hdr, inout my_meta_t my_meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control deparser(packet_out b, in parsed_packet_t hdr) {
    apply {
    }
}

control verify_c(inout parsed_packet_t hdr, inout my_meta_t my_meta) {
    apply {
    }
}

control compute_c(inout parsed_packet_t hdr, inout my_meta_t my_meta) {
    apply {
    }
}

V1Switch<parsed_packet_t, my_meta_t>(parse(), verify_c(), ingress(), egress(), compute_c(), deparser()) main;

