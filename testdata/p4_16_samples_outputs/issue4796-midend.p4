#include <core.p4>
#include <dpdk/pna.p4>

bit<3> max(in bit<3> val, in bit<3> bound) {
    @name("tmp") bit<3> tmp;
    if (val < bound) {
        tmp = val;
    } else {
        tmp = bound;
    }
    return tmp;
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
    return 8w0;
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
    bit<3> hsiVar;
    @name("MainControlImpl.revie") action revie(@name("orde") bit<128> orde, @name("priv") bit<128> priv) {
        hsiVar = max((bit<3>)shouldr(), 3w3);
        if (hsiVar == 3w0) {
            hdr.heal[3w0] = year_0;
        } else if (hsiVar == 3w1) {
            hdr.heal[3w1] = year_0;
        } else if (hsiVar == 3w2) {
            hdr.heal[3w2] = year_0;
        } else if (hsiVar == 3w3) {
            hdr.heal[3w3] = year_0;
        }
    }
    @name("MainControlImpl.greatY") table greatY_0 {
        actions = {
            revie();
            @defaultonly NoAction_1();
        }
        default_action = NoAction_1();
    }
    @hidden action act() {
        tmp_0 = true;
    }
    @hidden action act_0() {
        tmp_0 = false;
    }
    @hidden action issue4796l47() {
        tmp_1 = 16w48951;
    }
    @hidden action issue4796l47_0() {
        tmp_1 = 16w0;
    }
    @hidden action issue4796l47_1() {
        hdr.eth_hdr.eth_type = tmp_1;
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    @hidden table tbl_act_0 {
        actions = {
            act_0();
        }
        const default_action = act_0();
    }
    @hidden table tbl_issue4796l47 {
        actions = {
            issue4796l47();
        }
        const default_action = issue4796l47();
    }
    @hidden table tbl_issue4796l47_0 {
        actions = {
            issue4796l47_0();
        }
        const default_action = issue4796l47_0();
    }
    @hidden table tbl_issue4796l47_1 {
        actions = {
            issue4796l47_1();
        }
        const default_action = issue4796l47_1();
    }
    apply {
        if (greatY_0.apply().hit) {
            tbl_act.apply();
        } else {
            tbl_act_0.apply();
        }
        if (tmp_0) {
            tbl_issue4796l47.apply();
        } else {
            tbl_issue4796l47_0.apply();
        }
        tbl_issue4796l47_1.apply();
    }
}

control MainDeparserImpl(packet_out pkt, in Headers hdr, in main_metadata_t user_meta, in pna_main_output_metadata_t ostd) {
    apply {
    }
}

PNA_NIC<Headers, main_metadata_t, Headers, main_metadata_t>(MainParserImpl(), PreControlImpl(), MainControlImpl(), MainDeparserImpl()) main;
