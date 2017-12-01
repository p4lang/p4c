#include <core.p4>
#include <v1model.p4>

header instr_h {
    bit<8> op_code;
    bit<8> data;
}

header data_h {
    bit<8> data;
}

struct my_packet {
    instr_h[4] instr;
    data_h[4]  data;
}

struct my_metadata {
}

parser MyParser(packet_in b, out my_packet p, inout my_metadata m, inout standard_metadata_t s) {
    state start {
        transition parse_instr;
    }
    state parse_instr {
        b.extract(p.stack.next);
        transition select(p.stack.last.op_code) {
            0: parse_data;
            default: parse_instr;
        }
    }
    state parse_data {
        b.extract(p.stack.next);
        transition select(p.stack.last.op_code) {
            0: accept;
            default: parse_data;
        }
    }
}

control MyVerifyChecksum(inout my_packet hdr, inout my_metadata meta) {
    apply {
    }
}

control MyIngress(inout my_packet p, inout my_metadata m, inout standard_metadata_t s) {
    action nop() {
    }
    table t {
        key = {
            p.stack[0].op_code: exact;
        }
        actions = {
            nop;
        }
        default_action = nop;
    }
    apply {
        t.apply();
    }
}

control MyEgress(inout my_packet p, inout my_metadata m, inout standard_metadata_t s) {
    apply {
    }
}

control MyComputeChecksum(inout my_packet p, inout my_metadata m) {
    apply {
    }
}

control MyDeparser(packet_out b, in my_packet p) {
    apply {
        b.emit(p.stack);
    }
}

V1Switch(MyParser(), MyVerifyChecksum(), MyIngress(), MyEgress(), MyComputeChecksum(), MyDeparser()) main;

