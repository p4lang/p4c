control proto<P>(inout P pkt);
package top<P>(proto<P> p);
header data_t {
    bit<8>[2][2] arr;
    bit<1>       w;
    bit<1>       x;
    bit<1>       y;
    bit<1>       z;
}

struct headers {
    data_t data;
}

control c(inout headers hdr) {
    bit<1> hsiVar;
    bit<1> hsiVar_0;
    bit<1> hsiVar_1;
    bit<8> hsVar;
    bit<1> hsiVar_2;
    @hidden action array5l19() {
        hdr.data.arr[1w0][1w0] = hdr.data.arr[1w0][1w0];
    }
    @hidden action array5l19_0() {
        hdr.data.arr[1w0][1w0] = hdr.data.arr[1w0][1w1];
    }
    @hidden action array5l19_1() {
        hdr.data.arr[1w0][1w0] = hsVar;
    }
    @hidden action array5l19_2() {
        hsiVar_2 = hdr.data.z;
    }
    @hidden action array5l19_3() {
        hdr.data.arr[1w0][1w0] = hdr.data.arr[1w1][1w0];
    }
    @hidden action array5l19_4() {
        hdr.data.arr[1w0][1w0] = hdr.data.arr[1w1][1w1];
    }
    @hidden action array5l19_5() {
        hdr.data.arr[1w0][1w0] = hsVar;
    }
    @hidden action array5l19_6() {
        hsiVar_2 = hdr.data.z;
    }
    @hidden action array5l19_7() {
        hdr.data.arr[1w0][1w0] = hsVar;
    }
    @hidden action array5l19_8() {
        hsiVar_1 = hdr.data.y;
    }
    @hidden action array5l19_9() {
        hdr.data.arr[1w0][1w1] = hdr.data.arr[1w0][1w0];
    }
    @hidden action array5l19_10() {
        hdr.data.arr[1w0][1w1] = hdr.data.arr[1w0][1w1];
    }
    @hidden action array5l19_11() {
        hdr.data.arr[1w0][1w1] = hsVar;
    }
    @hidden action array5l19_12() {
        hsiVar_2 = hdr.data.z;
    }
    @hidden action array5l19_13() {
        hdr.data.arr[1w0][1w1] = hdr.data.arr[1w1][1w0];
    }
    @hidden action array5l19_14() {
        hdr.data.arr[1w0][1w1] = hdr.data.arr[1w1][1w1];
    }
    @hidden action array5l19_15() {
        hdr.data.arr[1w0][1w1] = hsVar;
    }
    @hidden action array5l19_16() {
        hsiVar_2 = hdr.data.z;
    }
    @hidden action array5l19_17() {
        hdr.data.arr[1w0][1w1] = hsVar;
    }
    @hidden action array5l19_18() {
        hsiVar_1 = hdr.data.y;
    }
    @hidden action array5l19_19() {
        hsiVar_0 = hdr.data.x;
    }
    @hidden action array5l19_20() {
        hdr.data.arr[1w1][1w0] = hdr.data.arr[1w0][1w0];
    }
    @hidden action array5l19_21() {
        hdr.data.arr[1w1][1w0] = hdr.data.arr[1w0][1w1];
    }
    @hidden action array5l19_22() {
        hdr.data.arr[1w1][1w0] = hsVar;
    }
    @hidden action array5l19_23() {
        hsiVar_2 = hdr.data.z;
    }
    @hidden action array5l19_24() {
        hdr.data.arr[1w1][1w0] = hdr.data.arr[1w1][1w0];
    }
    @hidden action array5l19_25() {
        hdr.data.arr[1w1][1w0] = hdr.data.arr[1w1][1w1];
    }
    @hidden action array5l19_26() {
        hdr.data.arr[1w1][1w0] = hsVar;
    }
    @hidden action array5l19_27() {
        hsiVar_2 = hdr.data.z;
    }
    @hidden action array5l19_28() {
        hdr.data.arr[1w1][1w0] = hsVar;
    }
    @hidden action array5l19_29() {
        hsiVar_1 = hdr.data.y;
    }
    @hidden action array5l19_30() {
        hdr.data.arr[1w1][1w1] = hdr.data.arr[1w0][1w0];
    }
    @hidden action array5l19_31() {
        hdr.data.arr[1w1][1w1] = hdr.data.arr[1w0][1w1];
    }
    @hidden action array5l19_32() {
        hdr.data.arr[1w1][1w1] = hsVar;
    }
    @hidden action array5l19_33() {
        hsiVar_2 = hdr.data.z;
    }
    @hidden action array5l19_34() {
        hdr.data.arr[1w1][1w1] = hdr.data.arr[1w1][1w0];
    }
    @hidden action array5l19_35() {
        hdr.data.arr[1w1][1w1] = hdr.data.arr[1w1][1w1];
    }
    @hidden action array5l19_36() {
        hdr.data.arr[1w1][1w1] = hsVar;
    }
    @hidden action array5l19_37() {
        hsiVar_2 = hdr.data.z;
    }
    @hidden action array5l19_38() {
        hdr.data.arr[1w1][1w1] = hsVar;
    }
    @hidden action array5l19_39() {
        hsiVar_1 = hdr.data.y;
    }
    @hidden action array5l19_40() {
        hsiVar_0 = hdr.data.x;
    }
    @hidden action array5l19_41() {
        hsiVar = hdr.data.w;
    }
    @hidden table tbl_array5l19 {
        actions = {
            array5l19_41();
        }
        const default_action = array5l19_41();
    }
    @hidden table tbl_array5l19_0 {
        actions = {
            array5l19_19();
        }
        const default_action = array5l19_19();
    }
    @hidden table tbl_array5l19_1 {
        actions = {
            array5l19_8();
        }
        const default_action = array5l19_8();
    }
    @hidden table tbl_array5l19_2 {
        actions = {
            array5l19_2();
        }
        const default_action = array5l19_2();
    }
    @hidden table tbl_array5l19_3 {
        actions = {
            array5l19();
        }
        const default_action = array5l19();
    }
    @hidden table tbl_array5l19_4 {
        actions = {
            array5l19_0();
        }
        const default_action = array5l19_0();
    }
    @hidden table tbl_array5l19_5 {
        actions = {
            array5l19_1();
        }
        const default_action = array5l19_1();
    }
    @hidden table tbl_array5l19_6 {
        actions = {
            array5l19_6();
        }
        const default_action = array5l19_6();
    }
    @hidden table tbl_array5l19_7 {
        actions = {
            array5l19_3();
        }
        const default_action = array5l19_3();
    }
    @hidden table tbl_array5l19_8 {
        actions = {
            array5l19_4();
        }
        const default_action = array5l19_4();
    }
    @hidden table tbl_array5l19_9 {
        actions = {
            array5l19_5();
        }
        const default_action = array5l19_5();
    }
    @hidden table tbl_array5l19_10 {
        actions = {
            array5l19_7();
        }
        const default_action = array5l19_7();
    }
    @hidden table tbl_array5l19_11 {
        actions = {
            array5l19_18();
        }
        const default_action = array5l19_18();
    }
    @hidden table tbl_array5l19_12 {
        actions = {
            array5l19_12();
        }
        const default_action = array5l19_12();
    }
    @hidden table tbl_array5l19_13 {
        actions = {
            array5l19_9();
        }
        const default_action = array5l19_9();
    }
    @hidden table tbl_array5l19_14 {
        actions = {
            array5l19_10();
        }
        const default_action = array5l19_10();
    }
    @hidden table tbl_array5l19_15 {
        actions = {
            array5l19_11();
        }
        const default_action = array5l19_11();
    }
    @hidden table tbl_array5l19_16 {
        actions = {
            array5l19_16();
        }
        const default_action = array5l19_16();
    }
    @hidden table tbl_array5l19_17 {
        actions = {
            array5l19_13();
        }
        const default_action = array5l19_13();
    }
    @hidden table tbl_array5l19_18 {
        actions = {
            array5l19_14();
        }
        const default_action = array5l19_14();
    }
    @hidden table tbl_array5l19_19 {
        actions = {
            array5l19_15();
        }
        const default_action = array5l19_15();
    }
    @hidden table tbl_array5l19_20 {
        actions = {
            array5l19_17();
        }
        const default_action = array5l19_17();
    }
    @hidden table tbl_array5l19_21 {
        actions = {
            array5l19_40();
        }
        const default_action = array5l19_40();
    }
    @hidden table tbl_array5l19_22 {
        actions = {
            array5l19_29();
        }
        const default_action = array5l19_29();
    }
    @hidden table tbl_array5l19_23 {
        actions = {
            array5l19_23();
        }
        const default_action = array5l19_23();
    }
    @hidden table tbl_array5l19_24 {
        actions = {
            array5l19_20();
        }
        const default_action = array5l19_20();
    }
    @hidden table tbl_array5l19_25 {
        actions = {
            array5l19_21();
        }
        const default_action = array5l19_21();
    }
    @hidden table tbl_array5l19_26 {
        actions = {
            array5l19_22();
        }
        const default_action = array5l19_22();
    }
    @hidden table tbl_array5l19_27 {
        actions = {
            array5l19_27();
        }
        const default_action = array5l19_27();
    }
    @hidden table tbl_array5l19_28 {
        actions = {
            array5l19_24();
        }
        const default_action = array5l19_24();
    }
    @hidden table tbl_array5l19_29 {
        actions = {
            array5l19_25();
        }
        const default_action = array5l19_25();
    }
    @hidden table tbl_array5l19_30 {
        actions = {
            array5l19_26();
        }
        const default_action = array5l19_26();
    }
    @hidden table tbl_array5l19_31 {
        actions = {
            array5l19_28();
        }
        const default_action = array5l19_28();
    }
    @hidden table tbl_array5l19_32 {
        actions = {
            array5l19_39();
        }
        const default_action = array5l19_39();
    }
    @hidden table tbl_array5l19_33 {
        actions = {
            array5l19_33();
        }
        const default_action = array5l19_33();
    }
    @hidden table tbl_array5l19_34 {
        actions = {
            array5l19_30();
        }
        const default_action = array5l19_30();
    }
    @hidden table tbl_array5l19_35 {
        actions = {
            array5l19_31();
        }
        const default_action = array5l19_31();
    }
    @hidden table tbl_array5l19_36 {
        actions = {
            array5l19_32();
        }
        const default_action = array5l19_32();
    }
    @hidden table tbl_array5l19_37 {
        actions = {
            array5l19_37();
        }
        const default_action = array5l19_37();
    }
    @hidden table tbl_array5l19_38 {
        actions = {
            array5l19_34();
        }
        const default_action = array5l19_34();
    }
    @hidden table tbl_array5l19_39 {
        actions = {
            array5l19_35();
        }
        const default_action = array5l19_35();
    }
    @hidden table tbl_array5l19_40 {
        actions = {
            array5l19_36();
        }
        const default_action = array5l19_36();
    }
    @hidden table tbl_array5l19_41 {
        actions = {
            array5l19_38();
        }
        const default_action = array5l19_38();
    }
    apply {
        tbl_array5l19.apply();
        if (hsiVar == 1w0) {
            tbl_array5l19_0.apply();
            if (hsiVar_0 == 1w0) {
                tbl_array5l19_1.apply();
                if (hsiVar_1 == 1w0) {
                    tbl_array5l19_2.apply();
                    if (hsiVar_2 == 1w0) {
                        tbl_array5l19_3.apply();
                    } else if (hsiVar_2 == 1w1) {
                        tbl_array5l19_4.apply();
                    } else if (hsiVar_2 >= 1w1) {
                        tbl_array5l19_5.apply();
                    }
                } else if (hsiVar_1 == 1w1) {
                    tbl_array5l19_6.apply();
                    if (hsiVar_2 == 1w0) {
                        tbl_array5l19_7.apply();
                    } else if (hsiVar_2 == 1w1) {
                        tbl_array5l19_8.apply();
                    } else if (hsiVar_2 >= 1w1) {
                        tbl_array5l19_9.apply();
                    }
                } else if (hsiVar_1 >= 1w1) {
                    tbl_array5l19_10.apply();
                }
            } else if (hsiVar_0 == 1w1) {
                tbl_array5l19_11.apply();
                if (hsiVar_1 == 1w0) {
                    tbl_array5l19_12.apply();
                    if (hsiVar_2 == 1w0) {
                        tbl_array5l19_13.apply();
                    } else if (hsiVar_2 == 1w1) {
                        tbl_array5l19_14.apply();
                    } else if (hsiVar_2 >= 1w1) {
                        tbl_array5l19_15.apply();
                    }
                } else if (hsiVar_1 == 1w1) {
                    tbl_array5l19_16.apply();
                    if (hsiVar_2 == 1w0) {
                        tbl_array5l19_17.apply();
                    } else if (hsiVar_2 == 1w1) {
                        tbl_array5l19_18.apply();
                    } else if (hsiVar_2 >= 1w1) {
                        tbl_array5l19_19.apply();
                    }
                } else if (hsiVar_1 >= 1w1) {
                    tbl_array5l19_20.apply();
                }
            }
        } else if (hsiVar == 1w1) {
            tbl_array5l19_21.apply();
            if (hsiVar_0 == 1w0) {
                tbl_array5l19_22.apply();
                if (hsiVar_1 == 1w0) {
                    tbl_array5l19_23.apply();
                    if (hsiVar_2 == 1w0) {
                        tbl_array5l19_24.apply();
                    } else if (hsiVar_2 == 1w1) {
                        tbl_array5l19_25.apply();
                    } else if (hsiVar_2 >= 1w1) {
                        tbl_array5l19_26.apply();
                    }
                } else if (hsiVar_1 == 1w1) {
                    tbl_array5l19_27.apply();
                    if (hsiVar_2 == 1w0) {
                        tbl_array5l19_28.apply();
                    } else if (hsiVar_2 == 1w1) {
                        tbl_array5l19_29.apply();
                    } else if (hsiVar_2 >= 1w1) {
                        tbl_array5l19_30.apply();
                    }
                } else if (hsiVar_1 >= 1w1) {
                    tbl_array5l19_31.apply();
                }
            } else if (hsiVar_0 == 1w1) {
                tbl_array5l19_32.apply();
                if (hsiVar_1 == 1w0) {
                    tbl_array5l19_33.apply();
                    if (hsiVar_2 == 1w0) {
                        tbl_array5l19_34.apply();
                    } else if (hsiVar_2 == 1w1) {
                        tbl_array5l19_35.apply();
                    } else if (hsiVar_2 >= 1w1) {
                        tbl_array5l19_36.apply();
                    }
                } else if (hsiVar_1 == 1w1) {
                    tbl_array5l19_37.apply();
                    if (hsiVar_2 == 1w0) {
                        tbl_array5l19_38.apply();
                    } else if (hsiVar_2 == 1w1) {
                        tbl_array5l19_39.apply();
                    } else if (hsiVar_2 >= 1w1) {
                        tbl_array5l19_40.apply();
                    }
                } else if (hsiVar_1 >= 1w1) {
                    tbl_array5l19_41.apply();
                }
            }
        }
    }
}

top<headers>(c()) main;
