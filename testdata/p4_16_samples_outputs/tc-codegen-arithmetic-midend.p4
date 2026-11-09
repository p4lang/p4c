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
    @hidden action tccodegenarithmetic112() {
        drop_packet();
    }
    @hidden action tccodegenarithmetic30() {
        tmp = 64w1;
    }
    @hidden action tccodegenarithmetic30_0() {
        tmp = 64w0;
    }
    @hidden action tccodegenarithmetic30_1() {
        result_0 = tmp;
    }
    @hidden action tccodegenarithmetic29() {
        result_0 = 64w0;
    }
    @hidden action tccodegenarithmetic31() {
        tmp_0 = 64w1;
    }
    @hidden action tccodegenarithmetic31_0() {
        tmp_0 = 64w0;
    }
    @hidden action tccodegenarithmetic31_1() {
        result_0 = tmp_0;
    }
    @hidden action tccodegenarithmetic32() {
        result_0 = (bit<64>)((bit<32>)hdr.values.x + 32w4294967293);
    }
    @hidden action tccodegenarithmetic33() {
        result_0 = (bit<64>)((bit<32>)hdr.values.x - (bit<32>)hdr.values.y);
    }
    @hidden action tccodegenarithmetic34() {
        result_0 = (bit<64>)((bit<32>)hdr.values.x + 32w3);
    }
    @hidden action tccodegenarithmetic35() {
        result_0 = (bit<64>)((bit<32>)hdr.values.x + (bit<32>)hdr.values.y);
    }
    @hidden action tccodegenarithmetic36() {
        result_0 = (bit<64>)((bit<32>)hdr.values.x * (bit<32>)hdr.values.y);
    }
    @hidden action tccodegenarithmetic37() {
        result_0 = (bit<64>)(((bit<32>)hdr.values.x + 32w4294967293) * (bit<32>)hdr.values.y + 32w1);
    }
    @hidden action tccodegenarithmetic38() {
        result_0 = (bit<64>)((bit<32>)hdr.values.x + 32w4294967295);
    }
    @hidden action tccodegenarithmetic114() {
        hdr.values.result = result_0;
        send_to_port(32w1);
    }
    @hidden table tbl_tccodegenarithmetic112 {
        actions = {
            tccodegenarithmetic112();
        }
        const default_action = tccodegenarithmetic112();
    }
    @hidden table tbl_tccodegenarithmetic29 {
        actions = {
            tccodegenarithmetic29();
        }
        const default_action = tccodegenarithmetic29();
    }
    @hidden table tbl_tccodegenarithmetic30 {
        actions = {
            tccodegenarithmetic30();
        }
        const default_action = tccodegenarithmetic30();
    }
    @hidden table tbl_tccodegenarithmetic30_0 {
        actions = {
            tccodegenarithmetic30_0();
        }
        const default_action = tccodegenarithmetic30_0();
    }
    @hidden table tbl_tccodegenarithmetic30_1 {
        actions = {
            tccodegenarithmetic30_1();
        }
        const default_action = tccodegenarithmetic30_1();
    }
    @hidden table tbl_tccodegenarithmetic31 {
        actions = {
            tccodegenarithmetic31();
        }
        const default_action = tccodegenarithmetic31();
    }
    @hidden table tbl_tccodegenarithmetic31_0 {
        actions = {
            tccodegenarithmetic31_0();
        }
        const default_action = tccodegenarithmetic31_0();
    }
    @hidden table tbl_tccodegenarithmetic31_1 {
        actions = {
            tccodegenarithmetic31_1();
        }
        const default_action = tccodegenarithmetic31_1();
    }
    @hidden table tbl_tccodegenarithmetic32 {
        actions = {
            tccodegenarithmetic32();
        }
        const default_action = tccodegenarithmetic32();
    }
    @hidden table tbl_tccodegenarithmetic33 {
        actions = {
            tccodegenarithmetic33();
        }
        const default_action = tccodegenarithmetic33();
    }
    @hidden table tbl_tccodegenarithmetic34 {
        actions = {
            tccodegenarithmetic34();
        }
        const default_action = tccodegenarithmetic34();
    }
    @hidden table tbl_tccodegenarithmetic35 {
        actions = {
            tccodegenarithmetic35();
        }
        const default_action = tccodegenarithmetic35();
    }
    @hidden table tbl_tccodegenarithmetic36 {
        actions = {
            tccodegenarithmetic36();
        }
        const default_action = tccodegenarithmetic36();
    }
    @hidden table tbl_tccodegenarithmetic37 {
        actions = {
            tccodegenarithmetic37();
        }
        const default_action = tccodegenarithmetic37();
    }
    @hidden table tbl_tccodegenarithmetic38 {
        actions = {
            tccodegenarithmetic38();
        }
        const default_action = tccodegenarithmetic38();
    }
    @hidden table tbl_tccodegenarithmetic114 {
        actions = {
            tccodegenarithmetic114();
        }
        const default_action = tccodegenarithmetic114();
    }
    apply {
        if (istd.parser_error != error.NoError) {
            tbl_tccodegenarithmetic112.apply();
        } else {
            tbl_tccodegenarithmetic29.apply();
            if (hdr.values.op == 8w0) {
                if ((bit<32>)hdr.values.x > (bit<32>)hdr.values.y + 32w4294967282) {
                    tbl_tccodegenarithmetic30.apply();
                } else {
                    tbl_tccodegenarithmetic30_0.apply();
                }
                tbl_tccodegenarithmetic30_1.apply();
            }
            if (hdr.values.op == 8w1) {
                if ((bit<32>)hdr.values.x + 32w4294967276 != (bit<32>)hdr.values.y) {
                    tbl_tccodegenarithmetic31.apply();
                } else {
                    tbl_tccodegenarithmetic31_0.apply();
                }
                tbl_tccodegenarithmetic31_1.apply();
            }
            if (hdr.values.op == 8w2) {
                tbl_tccodegenarithmetic32.apply();
            }
            if (hdr.values.op == 8w3) {
                tbl_tccodegenarithmetic33.apply();
            }
            if (hdr.values.op == 8w4) {
                tbl_tccodegenarithmetic34.apply();
            }
            if (hdr.values.op == 8w5) {
                tbl_tccodegenarithmetic35.apply();
            }
            if (hdr.values.op == 8w6) {
                tbl_tccodegenarithmetic36.apply();
            }
            if (hdr.values.op == 8w7) {
                tbl_tccodegenarithmetic37.apply();
            }
            if (hdr.values.op == 8w8) {
                tbl_tccodegenarithmetic38.apply();
            }
            tbl_tccodegenarithmetic114.apply();
        }
    }
}

control Deparser(packet_out pkt, inout headers_t hdr, in metadata_t meta, in pna_main_output_metadata_t ostd) {
    @hidden action tccodegenarithmetic129() {
        pkt.emit<ethernet_t>(hdr.ethernet);
        pkt.emit<values_t>(hdr.values);
    }
    @hidden table tbl_tccodegenarithmetic129 {
        actions = {
            tccodegenarithmetic129();
        }
        const default_action = tccodegenarithmetic129();
    }
    apply {
        tbl_tccodegenarithmetic129.apply();
    }
}

PNA_NIC<headers_t, metadata_t>(Parser(), Ingress(), Deparser()) main;
