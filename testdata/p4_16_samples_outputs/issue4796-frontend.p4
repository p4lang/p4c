#include <core.p4>
#include <dpdk/pna.p4>

bit<3> max(in bit<3> val, in bit<3> bound) {
    @name("retval") bit<3> retval;
    @name("tmp") bit<3> tmp;
    if (val < bound) {
        tmp = val;
    } else {
        tmp = bound;
    }
    retval = tmp;
    return retval;
}
header ethernet_t {
    bit<16> eth_type;
}

header priceX {
}

struct Headers {
    ethernet_t eth_hdr;
    priceX[4]  heal;
}

struct main_metadata_t {
}

bit<8> shouldr() {
    @name("retval_0") bit<8> retval_0;
    retval_0 = 8w0;
    return retval_0;
}
parser MainParserImpl(packet_in pkt, out Headers hdr, inout main_metadata_t user_meta, in pna_main_parser_input_metadata_t istd) {
    state start {
        transition reject;
    }
}

control PreControlImpl(in Headers hdr, inout main_metadata_t user_meta, in pna_pre_input_metadata_t istd, inout pna_pre_output_metadata_t ostd) {
    apply {
    }
}

control MainControlImpl(inout Headers hdr, inout main_metadata_t user_meta, in pna_main_input_metadata_t istd, inout pna_main_output_metadata_t ostd) {
    @name("MainControlImpl.tmp_0") bool tmp_0;
    @name("MainControlImpl.tmp_1") bit<16> tmp_1;
    @name("MainControlImpl.year") priceX year_0;
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("MainControlImpl.revie") action revie(@name("orde") bit<128> orde, @name("priv") bit<128> priv) {
        hdr.heal[max((bit<3>)shouldr(), 3w3)] = year_0;
    }
    @name("MainControlImpl.greatY") table greatY_0 {
        actions = {
            revie();
            @defaultonly NoAction_1();
        }
        default_action = NoAction_1();
    }
    apply {
        tmp_0 = greatY_0.apply().hit;
        if (tmp_0) {
            tmp_1 = 16w48951;
        } else {
            tmp_1 = 16w0;
        }
        hdr.eth_hdr.eth_type = tmp_1;
    }
}

control MainDeparserImpl(packet_out pkt, in Headers hdr, in main_metadata_t user_meta, in pna_main_output_metadata_t ostd) {
    apply {
    }
}

PNA_NIC<Headers, main_metadata_t, Headers, main_metadata_t>(MainParserImpl(), PreControlImpl(), MainControlImpl(), MainDeparserImpl()) main;
