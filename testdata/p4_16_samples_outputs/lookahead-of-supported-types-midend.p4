#include <core.p4>

header header_t {
    bool   f1;
    bit<8> f2;
    int<8> f3;
    bit<8> f4;
}

struct struct_t {
    bool        f1;
    bit<8>      f2;
    int<8>      f3;
    bit<8>      f4;
    header_t    f5;
    header_t[8] f6;
}

struct metadata_t {
    bool        _bool_f0;
    bit<8>      _bit_f1;
    int<8>      _int_f2;
    bit<8>      _enum_f3;
    header_t    _header_f4;
    header_t[8] _header_stack_f5;
    bool        _struct_f_f16;
    bit<8>      _struct_f_f27;
    int<8>      _struct_f_f38;
    bit<8>      _struct_f_f49;
    header_t    _struct_f_f510;
    header_t[8] _struct_f_f611;
}

parser TestParser(packet_in pkt, inout metadata_t meta) {
    bit<1> tmp;
    bit<8> tmp_0;
    bit<8> tmp_1;
    bit<8> tmp_2;
    bit<25> tmp_3;
    bit<200> tmp_4;
    bit<250> tmp_5;
    state start {
        tmp = pkt.lookahead<bit<1>>();
        meta._bool_f0 = (bool)tmp;
        tmp_0 = pkt.lookahead<bit<8>>();
        meta._bit_f1 = tmp_0;
        tmp_1 = pkt.lookahead<bit<8>>();
        meta._int_f2 = (int<8>)tmp_1;
        tmp_2 = pkt.lookahead<bit<8>>();
        meta._enum_f3 = tmp_2;
        tmp_3 = pkt.lookahead<bit<25>>();
        meta._header_f4.setValid();
        meta._header_f4.f1 = (bool)tmp_3[24:24];
        meta._header_f4.f2 = tmp_3[23:16];
        meta._header_f4.f3 = (int<8>)tmp_3[15:8];
        meta._header_f4.f4 = tmp_3[7:0];
        tmp_4 = pkt.lookahead<bit<200>>();
        meta._header_stack_f5[0].setValid();
        meta._header_stack_f5[0].f1 = (bool)tmp_4[199:199];
        meta._header_stack_f5[0].f2 = tmp_4[198:191];
        meta._header_stack_f5[0].f3 = (int<8>)tmp_4[190:183];
        meta._header_stack_f5[0].f4 = tmp_4[182:175];
        meta._header_stack_f5[1].setValid();
        meta._header_stack_f5[1].f1 = (bool)tmp_4[174:174];
        meta._header_stack_f5[1].f2 = tmp_4[173:166];
        meta._header_stack_f5[1].f3 = (int<8>)tmp_4[165:158];
        meta._header_stack_f5[1].f4 = tmp_4[157:150];
        meta._header_stack_f5[2].setValid();
        meta._header_stack_f5[2].f1 = (bool)tmp_4[149:149];
        meta._header_stack_f5[2].f2 = tmp_4[148:141];
        meta._header_stack_f5[2].f3 = (int<8>)tmp_4[140:133];
        meta._header_stack_f5[2].f4 = tmp_4[132:125];
        meta._header_stack_f5[3].setValid();
        meta._header_stack_f5[3].f1 = (bool)tmp_4[124:124];
        meta._header_stack_f5[3].f2 = tmp_4[123:116];
        meta._header_stack_f5[3].f3 = (int<8>)tmp_4[115:108];
        meta._header_stack_f5[3].f4 = tmp_4[107:100];
        meta._header_stack_f5[4].setValid();
        meta._header_stack_f5[4].f1 = (bool)tmp_4[99:99];
        meta._header_stack_f5[4].f2 = tmp_4[98:91];
        meta._header_stack_f5[4].f3 = (int<8>)tmp_4[90:83];
        meta._header_stack_f5[4].f4 = tmp_4[82:75];
        meta._header_stack_f5[5].setValid();
        meta._header_stack_f5[5].f1 = (bool)tmp_4[74:74];
        meta._header_stack_f5[5].f2 = tmp_4[73:66];
        meta._header_stack_f5[5].f3 = (int<8>)tmp_4[65:58];
        meta._header_stack_f5[5].f4 = tmp_4[57:50];
        meta._header_stack_f5[6].setValid();
        meta._header_stack_f5[6].f1 = (bool)tmp_4[49:49];
        meta._header_stack_f5[6].f2 = tmp_4[48:41];
        meta._header_stack_f5[6].f3 = (int<8>)tmp_4[40:33];
        meta._header_stack_f5[6].f4 = tmp_4[32:25];
        meta._header_stack_f5[7].setValid();
        meta._header_stack_f5[7].f1 = (bool)tmp_4[24:24];
        meta._header_stack_f5[7].f2 = tmp_4[23:16];
        meta._header_stack_f5[7].f3 = (int<8>)tmp_4[15:8];
        meta._header_stack_f5[7].f4 = tmp_4[7:0];
        tmp_5 = pkt.lookahead<bit<250>>();
        meta._struct_f_f16 = (bool)tmp_5[249:249];
        meta._struct_f_f27 = tmp_5[248:241];
        meta._struct_f_f38 = (int<8>)tmp_5[240:233];
        meta._struct_f_f49 = tmp_5[232:225];
        meta._struct_f_f510.setValid();
        meta._struct_f_f510.f1 = (bool)tmp_5[224:224];
        meta._struct_f_f510.f2 = tmp_5[223:216];
        meta._struct_f_f510.f3 = (int<8>)tmp_5[215:208];
        meta._struct_f_f510.f4 = tmp_5[207:200];
        meta._struct_f_f611[0].setValid();
        meta._struct_f_f611[0].f1 = (bool)tmp_5[199:199];
        meta._struct_f_f611[0].f2 = tmp_5[198:191];
        meta._struct_f_f611[0].f3 = (int<8>)tmp_5[190:183];
        meta._struct_f_f611[0].f4 = tmp_5[182:175];
        meta._struct_f_f611[1].setValid();
        meta._struct_f_f611[1].f1 = (bool)tmp_5[174:174];
        meta._struct_f_f611[1].f2 = tmp_5[173:166];
        meta._struct_f_f611[1].f3 = (int<8>)tmp_5[165:158];
        meta._struct_f_f611[1].f4 = tmp_5[157:150];
        meta._struct_f_f611[2].setValid();
        meta._struct_f_f611[2].f1 = (bool)tmp_5[149:149];
        meta._struct_f_f611[2].f2 = tmp_5[148:141];
        meta._struct_f_f611[2].f3 = (int<8>)tmp_5[140:133];
        meta._struct_f_f611[2].f4 = tmp_5[132:125];
        meta._struct_f_f611[3].setValid();
        meta._struct_f_f611[3].f1 = (bool)tmp_5[124:124];
        meta._struct_f_f611[3].f2 = tmp_5[123:116];
        meta._struct_f_f611[3].f3 = (int<8>)tmp_5[115:108];
        meta._struct_f_f611[3].f4 = tmp_5[107:100];
        meta._struct_f_f611[4].setValid();
        meta._struct_f_f611[4].f1 = (bool)tmp_5[99:99];
        meta._struct_f_f611[4].f2 = tmp_5[98:91];
        meta._struct_f_f611[4].f3 = (int<8>)tmp_5[90:83];
        meta._struct_f_f611[4].f4 = tmp_5[82:75];
        meta._struct_f_f611[5].setValid();
        meta._struct_f_f611[5].f1 = (bool)tmp_5[74:74];
        meta._struct_f_f611[5].f2 = tmp_5[73:66];
        meta._struct_f_f611[5].f3 = (int<8>)tmp_5[65:58];
        meta._struct_f_f611[5].f4 = tmp_5[57:50];
        meta._struct_f_f611[6].setValid();
        meta._struct_f_f611[6].f1 = (bool)tmp_5[49:49];
        meta._struct_f_f611[6].f2 = tmp_5[48:41];
        meta._struct_f_f611[6].f3 = (int<8>)tmp_5[40:33];
        meta._struct_f_f611[6].f4 = tmp_5[32:25];
        meta._struct_f_f611[7].setValid();
        meta._struct_f_f611[7].f1 = (bool)tmp_5[24:24];
        meta._struct_f_f611[7].f2 = tmp_5[23:16];
        meta._struct_f_f611[7].f3 = (int<8>)tmp_5[15:8];
        meta._struct_f_f611[7].f4 = tmp_5[7:0];
        transition accept;
    }
}

parser Parser<M>(packet_in pkt, inout M meta);
package P<M>(Parser<M> _p);
P<metadata_t>(TestParser()) main;
