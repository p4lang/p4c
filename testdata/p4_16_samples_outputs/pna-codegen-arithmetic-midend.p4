#include <core.p4>
#include <tc/pna.p4>

header ethernet_t {
    bit<48> dst;
    bit<48> src;
    bit<16> kind;
}

header values_t {
    bit<8>  op;
    bit<64> x;
    bit<64> y;
    bit<64> result;
}

struct headers_t {
    ethernet_t ethernet;
    values_t   values;
}

struct metadata_t {
    bit<8> unused;
}

parser Parser(packet_in pkt, out headers_t hdr, inout metadata_t meta, in pna_main_parser_input_metadata_t istd) {
    state start {
        pkt.extract<ethernet_t>(hdr.ethernet);
        verify(hdr.ethernet.kind == 16w0x88b5, error.NoMatch);
        pkt.extract<values_t>(hdr.values);
        transition accept;
    }
}

control Ingress(inout headers_t hdr, inout metadata_t meta, in pna_main_input_metadata_t istd, inout pna_main_output_metadata_t ostd) {
    @name("Ingress.tmp") bit<64> tmp;
    @name("Ingress.tmp_0") bit<64> tmp_0;
    @name("Ingress.result") bit<64> result_0;
    @hidden action pnacodegenarithmetic112() {
        drop_packet();
    }
    @hidden action pnacodegenarithmetic30() {
        tmp = 64w1;
    }
    @hidden action pnacodegenarithmetic30_0() {
        tmp = 64w0;
    }
    @hidden action pnacodegenarithmetic30_1() {
        result_0 = tmp;
    }
    @hidden action pnacodegenarithmetic29() {
        result_0 = 64w0;
    }
    @hidden action pnacodegenarithmetic31() {
        tmp_0 = 64w1;
    }
    @hidden action pnacodegenarithmetic31_0() {
        tmp_0 = 64w0;
    }
    @hidden action pnacodegenarithmetic31_1() {
        result_0 = tmp_0;
    }
    @hidden action pnacodegenarithmetic32() {
        result_0 = (bit<64>)((bit<32>)hdr.values.x + 32w4294967293);
    }
    @hidden action pnacodegenarithmetic33() {
        result_0 = (bit<64>)((bit<32>)hdr.values.x - (bit<32>)hdr.values.y);
    }
    @hidden action pnacodegenarithmetic34() {
        result_0 = (bit<64>)((bit<32>)hdr.values.x + 32w3);
    }
    @hidden action pnacodegenarithmetic35() {
        result_0 = (bit<64>)((bit<32>)hdr.values.x + (bit<32>)hdr.values.y);
    }
    @hidden action pnacodegenarithmetic36() {
        result_0 = (bit<64>)((bit<32>)hdr.values.x * (bit<32>)hdr.values.y);
    }
    @hidden action pnacodegenarithmetic37() {
        result_0 = (bit<64>)(((bit<32>)hdr.values.x + 32w4294967293) * (bit<32>)hdr.values.y + 32w1);
    }
    @hidden action pnacodegenarithmetic38() {
        result_0 = (bit<64>)((bit<32>)hdr.values.x + 32w4294967295);
    }
    @hidden action pnacodegenarithmetic114() {
        hdr.values.result = result_0;
        send_to_port(32w1);
    }
    @hidden table tbl_pnacodegenarithmetic112 {
        actions = {
            pnacodegenarithmetic112();
        }
        const default_action = pnacodegenarithmetic112();
    }
    @hidden table tbl_pnacodegenarithmetic29 {
        actions = {
            pnacodegenarithmetic29();
        }
        const default_action = pnacodegenarithmetic29();
    }
    @hidden table tbl_pnacodegenarithmetic30 {
        actions = {
            pnacodegenarithmetic30();
        }
        const default_action = pnacodegenarithmetic30();
    }
    @hidden table tbl_pnacodegenarithmetic30_0 {
        actions = {
            pnacodegenarithmetic30_0();
        }
        const default_action = pnacodegenarithmetic30_0();
    }
    @hidden table tbl_pnacodegenarithmetic30_1 {
        actions = {
            pnacodegenarithmetic30_1();
        }
        const default_action = pnacodegenarithmetic30_1();
    }
    @hidden table tbl_pnacodegenarithmetic31 {
        actions = {
            pnacodegenarithmetic31();
        }
        const default_action = pnacodegenarithmetic31();
    }
    @hidden table tbl_pnacodegenarithmetic31_0 {
        actions = {
            pnacodegenarithmetic31_0();
        }
        const default_action = pnacodegenarithmetic31_0();
    }
    @hidden table tbl_pnacodegenarithmetic31_1 {
        actions = {
            pnacodegenarithmetic31_1();
        }
        const default_action = pnacodegenarithmetic31_1();
    }
    @hidden table tbl_pnacodegenarithmetic32 {
        actions = {
            pnacodegenarithmetic32();
        }
        const default_action = pnacodegenarithmetic32();
    }
    @hidden table tbl_pnacodegenarithmetic33 {
        actions = {
            pnacodegenarithmetic33();
        }
        const default_action = pnacodegenarithmetic33();
    }
    @hidden table tbl_pnacodegenarithmetic34 {
        actions = {
            pnacodegenarithmetic34();
        }
        const default_action = pnacodegenarithmetic34();
    }
    @hidden table tbl_pnacodegenarithmetic35 {
        actions = {
            pnacodegenarithmetic35();
        }
        const default_action = pnacodegenarithmetic35();
    }
    @hidden table tbl_pnacodegenarithmetic36 {
        actions = {
            pnacodegenarithmetic36();
        }
        const default_action = pnacodegenarithmetic36();
    }
    @hidden table tbl_pnacodegenarithmetic37 {
        actions = {
            pnacodegenarithmetic37();
        }
        const default_action = pnacodegenarithmetic37();
    }
    @hidden table tbl_pnacodegenarithmetic38 {
        actions = {
            pnacodegenarithmetic38();
        }
        const default_action = pnacodegenarithmetic38();
    }
    @hidden table tbl_pnacodegenarithmetic114 {
        actions = {
            pnacodegenarithmetic114();
        }
        const default_action = pnacodegenarithmetic114();
    }
    apply {
        if (istd.parser_error != error.NoError) {
            tbl_pnacodegenarithmetic112.apply();
        } else {
            tbl_pnacodegenarithmetic29.apply();
            if (hdr.values.op == 8w0) {
                if ((bit<32>)hdr.values.x > (bit<32>)hdr.values.y + 32w4294967282) {
                    tbl_pnacodegenarithmetic30.apply();
                } else {
                    tbl_pnacodegenarithmetic30_0.apply();
                }
                tbl_pnacodegenarithmetic30_1.apply();
            }
            if (hdr.values.op == 8w1) {
                if ((bit<32>)hdr.values.x + 32w4294967276 != (bit<32>)hdr.values.y) {
                    tbl_pnacodegenarithmetic31.apply();
                } else {
                    tbl_pnacodegenarithmetic31_0.apply();
                }
                tbl_pnacodegenarithmetic31_1.apply();
            }
            if (hdr.values.op == 8w2) {
                tbl_pnacodegenarithmetic32.apply();
            }
            if (hdr.values.op == 8w3) {
                tbl_pnacodegenarithmetic33.apply();
            }
            if (hdr.values.op == 8w4) {
                tbl_pnacodegenarithmetic34.apply();
            }
            if (hdr.values.op == 8w5) {
                tbl_pnacodegenarithmetic35.apply();
            }
            if (hdr.values.op == 8w6) {
                tbl_pnacodegenarithmetic36.apply();
            }
            if (hdr.values.op == 8w7) {
                tbl_pnacodegenarithmetic37.apply();
            }
            if (hdr.values.op == 8w8) {
                tbl_pnacodegenarithmetic38.apply();
            }
            tbl_pnacodegenarithmetic114.apply();
        }
    }
}

control Deparser(packet_out pkt, inout headers_t hdr, in metadata_t meta, in pna_main_output_metadata_t ostd) {
    @hidden action pnacodegenarithmetic129() {
        pkt.emit<ethernet_t>(hdr.ethernet);
        pkt.emit<values_t>(hdr.values);
    }
    @hidden table tbl_pnacodegenarithmetic129 {
        actions = {
            pnacodegenarithmetic129();
        }
        const default_action = pnacodegenarithmetic129();
    }
    apply {
        tbl_pnacodegenarithmetic129.apply();
    }
}

PNA_NIC<headers_t, metadata_t>(Parser(), Ingress(), Deparser()) main;
