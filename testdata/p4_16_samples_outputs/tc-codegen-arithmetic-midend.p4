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
    @hidden action tccodegenarithmetic108() {
        drop_packet();
    }
    @hidden action tccodegenarithmetic26() {
        tmp = 64w1;
    }
    @hidden action tccodegenarithmetic26_0() {
        tmp = 64w0;
    }
    @hidden action tccodegenarithmetic26_1() {
        result_0 = tmp;
    }
    @hidden action tccodegenarithmetic25() {
        result_0 = 64w0;
    }
    @hidden action tccodegenarithmetic27() {
        tmp_0 = 64w1;
    }
    @hidden action tccodegenarithmetic27_0() {
        tmp_0 = 64w0;
    }
    @hidden action tccodegenarithmetic27_1() {
        result_0 = tmp_0;
    }
    @hidden action tccodegenarithmetic28() {
        result_0 = (bit<64>)((bit<32>)hdr.values.x + 32w4294967293);
    }
    @hidden action tccodegenarithmetic29() {
        result_0 = (bit<64>)((bit<32>)hdr.values.x - (bit<32>)hdr.values.y);
    }
    @hidden action tccodegenarithmetic30() {
        result_0 = (bit<64>)((bit<32>)hdr.values.x + 32w3);
    }
    @hidden action tccodegenarithmetic31() {
        result_0 = (bit<64>)((bit<32>)hdr.values.x + (bit<32>)hdr.values.y);
    }
    @hidden action tccodegenarithmetic32() {
        result_0 = (bit<64>)((bit<32>)hdr.values.x * (bit<32>)hdr.values.y);
    }
    @hidden action tccodegenarithmetic33() {
        result_0 = (bit<64>)(((bit<32>)hdr.values.x + 32w4294967293) * (bit<32>)hdr.values.y + 32w1);
    }
    @hidden action tccodegenarithmetic34() {
        result_0 = (bit<64>)((bit<32>)hdr.values.x + 32w4294967295);
    }
    @hidden action tccodegenarithmetic110() {
        hdr.values.result = result_0;
        send_to_port(istd.input_port);
    }
    @hidden table tbl_tccodegenarithmetic108 {
        actions = {
            tccodegenarithmetic108();
        }
        const default_action = tccodegenarithmetic108();
    }
    @hidden table tbl_tccodegenarithmetic25 {
        actions = {
            tccodegenarithmetic25();
        }
        const default_action = tccodegenarithmetic25();
    }
    @hidden table tbl_tccodegenarithmetic26 {
        actions = {
            tccodegenarithmetic26();
        }
        const default_action = tccodegenarithmetic26();
    }
    @hidden table tbl_tccodegenarithmetic26_0 {
        actions = {
            tccodegenarithmetic26_0();
        }
        const default_action = tccodegenarithmetic26_0();
    }
    @hidden table tbl_tccodegenarithmetic26_1 {
        actions = {
            tccodegenarithmetic26_1();
        }
        const default_action = tccodegenarithmetic26_1();
    }
    @hidden table tbl_tccodegenarithmetic27 {
        actions = {
            tccodegenarithmetic27();
        }
        const default_action = tccodegenarithmetic27();
    }
    @hidden table tbl_tccodegenarithmetic27_0 {
        actions = {
            tccodegenarithmetic27_0();
        }
        const default_action = tccodegenarithmetic27_0();
    }
    @hidden table tbl_tccodegenarithmetic27_1 {
        actions = {
            tccodegenarithmetic27_1();
        }
        const default_action = tccodegenarithmetic27_1();
    }
    @hidden table tbl_tccodegenarithmetic28 {
        actions = {
            tccodegenarithmetic28();
        }
        const default_action = tccodegenarithmetic28();
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
    @hidden table tbl_tccodegenarithmetic31 {
        actions = {
            tccodegenarithmetic31();
        }
        const default_action = tccodegenarithmetic31();
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
    @hidden table tbl_tccodegenarithmetic110 {
        actions = {
            tccodegenarithmetic110();
        }
        const default_action = tccodegenarithmetic110();
    }
    apply {
        if (istd.parser_error != error.NoError) {
            tbl_tccodegenarithmetic108.apply();
        } else {
            tbl_tccodegenarithmetic25.apply();
            if (hdr.values.op == 8w0) {
                if ((bit<32>)hdr.values.x > (bit<32>)hdr.values.y + 32w4294967282) {
                    tbl_tccodegenarithmetic26.apply();
                } else {
                    tbl_tccodegenarithmetic26_0.apply();
                }
                tbl_tccodegenarithmetic26_1.apply();
            }
            if (hdr.values.op == 8w1) {
                if ((bit<32>)hdr.values.x + 32w4294967276 != (bit<32>)hdr.values.y) {
                    tbl_tccodegenarithmetic27.apply();
                } else {
                    tbl_tccodegenarithmetic27_0.apply();
                }
                tbl_tccodegenarithmetic27_1.apply();
            }
            if (hdr.values.op == 8w2) {
                tbl_tccodegenarithmetic28.apply();
            }
            if (hdr.values.op == 8w3) {
                tbl_tccodegenarithmetic29.apply();
            }
            if (hdr.values.op == 8w4) {
                tbl_tccodegenarithmetic30.apply();
            }
            if (hdr.values.op == 8w5) {
                tbl_tccodegenarithmetic31.apply();
            }
            if (hdr.values.op == 8w6) {
                tbl_tccodegenarithmetic32.apply();
            }
            if (hdr.values.op == 8w7) {
                tbl_tccodegenarithmetic33.apply();
            }
            if (hdr.values.op == 8w8) {
                tbl_tccodegenarithmetic34.apply();
            }
            tbl_tccodegenarithmetic110.apply();
        }
    }
}

control Deparser(packet_out pkt, inout headers_t hdr, in metadata_t meta, in pna_main_output_metadata_t ostd) {
    @hidden action tccodegenarithmetic125() {
        pkt.emit<ethernet_t>(hdr.ethernet);
        pkt.emit<values_t>(hdr.values);
    }
    @hidden table tbl_tccodegenarithmetic125 {
        actions = {
            tccodegenarithmetic125();
        }
        const default_action = tccodegenarithmetic125();
    }
    apply {
        tbl_tccodegenarithmetic125.apply();
    }
}

PNA_NIC<headers_t, metadata_t>(Parser(), Ingress(), Deparser()) main;
