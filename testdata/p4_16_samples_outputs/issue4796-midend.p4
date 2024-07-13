#include <core.p4>
#include <dpdk/pna.p4>

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
    @name("MainControlImpl.tmp_0") bool tmp;
    @name("MainControlImpl.tmp_1") bit<16> tmp_0;
    @name("MainControlImpl.year") priceX year_0;
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("MainControlImpl.revie") action revie(@name("orde") bit<128> orde, @name("priv") bit<128> priv) {
        hdr.heal[3w0] = year_0;
    }
    @name("MainControlImpl.greatY") table greatY_0 {
        actions = {
            revie();
            @defaultonly NoAction_1();
        }
        default_action = NoAction_1();
    }
    @hidden action act() {
        tmp = true;
    }
    @hidden action act_0() {
        tmp = false;
    }
    @hidden action issue4796l47() {
        tmp_0 = 16w48951;
    }
    @hidden action issue4796l47_0() {
        tmp_0 = 16w0;
    }
    @hidden action issue4796l47_1() {
        hdr.eth_hdr.eth_type = tmp_0;
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
        if (tmp) {
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
