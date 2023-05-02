#include <core.p4>
#include <v1model.p4>

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
}

header H {
    bit<8> color_status;
    bit<32> idx;
}

struct headers {
    ethernet_t eth_hdr;
    H h;
}

enum bit<2> MeterColor {
    GREEN = 0,
    YELLOW = 1,
    RED = 2
}

struct Meta {}

parser p(packet_in pkt, out headers hdr, inout Meta m, inout standard_metadata_t sm) {
    state start {
        transition parse_hdrs;
    }
    state parse_hdrs {
        pkt.extract(hdr.eth_hdr);
        pkt.extract(hdr.h);
        transition accept;
    }
}

control ingress(inout headers h, inout Meta m, inout standard_metadata_t sm) {
    direct_meter<MeterColor>(MeterType.bytes) table_attached_meter;
    MeterColor color = (MeterColor) 0;

    action meter_assign() {
        table_attached_meter.read(color);
    }
    table meter_table {
        actions = {
            meter_assign;
        }
        key = {
            h.eth_hdr.dst_addr: ternary;
        }
        size = 16384;
        default_action = meter_assign();
        meters = table_attached_meter;
    }

    apply {
        meter_table.apply();
        if (color == MeterColor.RED) {
            h.h.color_status = 2;
        } else if (color == MeterColor.YELLOW) {
            h.h.color_status = 1;
        } else {
            h.h.color_status = 0;
        }
    }
}

control vrfy(inout headers h, inout Meta m) { apply {} }

control update(inout headers h, inout Meta m) { apply {} }

control egress(inout headers h, inout Meta m, inout standard_metadata_t sm) { apply {} }

control deparser(packet_out pkt, in headers h) {
    apply {
        pkt.emit(h);
    }
}
V1Switch(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
