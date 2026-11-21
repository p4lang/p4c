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
    @hidden action array5l25() {
        hdr.data.arr[1w0][1w0] = hdr.data.arr[1w0][1w0];
    }
    @hidden action array5l25_0() {
        hdr.data.arr[1w0][1w0] = hdr.data.arr[1w0][1w1];
    }
    @hidden action array5l25_1() {
        hdr.data.arr[1w0][1w0] = hsVar;
    }
    @hidden action array5l25_2() {
        hsiVar_2 = hdr.data.z;
    }
    @hidden action array5l25_3() {
        hdr.data.arr[1w0][1w0] = hdr.data.arr[1w1][1w0];
    }
    @hidden action array5l25_4() {
        hdr.data.arr[1w0][1w0] = hdr.data.arr[1w1][1w1];
    }
    @hidden action array5l25_5() {
        hdr.data.arr[1w0][1w0] = hsVar;
    }
    @hidden action array5l25_6() {
        hsiVar_2 = hdr.data.z;
    }
    @hidden action array5l25_7() {
        hdr.data.arr[1w0][1w0] = hsVar;
    }
    @hidden action array5l25_8() {
        hsiVar_1 = hdr.data.y;
    }
    @hidden action array5l25_9() {
        hdr.data.arr[1w0][1w1] = hdr.data.arr[1w0][1w0];
    }
    @hidden action array5l25_10() {
        hdr.data.arr[1w0][1w1] = hdr.data.arr[1w0][1w1];
    }
    @hidden action array5l25_11() {
        hdr.data.arr[1w0][1w1] = hsVar;
    }
    @hidden action array5l25_12() {
        hsiVar_2 = hdr.data.z;
    }
    @hidden action array5l25_13() {
        hdr.data.arr[1w0][1w1] = hdr.data.arr[1w1][1w0];
    }
    @hidden action array5l25_14() {
        hdr.data.arr[1w0][1w1] = hdr.data.arr[1w1][1w1];
    }
    @hidden action array5l25_15() {
        hdr.data.arr[1w0][1w1] = hsVar;
    }
    @hidden action array5l25_16() {
        hsiVar_2 = hdr.data.z;
    }
    @hidden action array5l25_17() {
        hdr.data.arr[1w0][1w1] = hsVar;
    }
    @hidden action array5l25_18() {
        hsiVar_1 = hdr.data.y;
    }
    @hidden action array5l25_19() {
        hsiVar_0 = hdr.data.x;
    }
    @hidden action array5l25_20() {
        hdr.data.arr[1w1][1w0] = hdr.data.arr[1w0][1w0];
    }
    @hidden action array5l25_21() {
        hdr.data.arr[1w1][1w0] = hdr.data.arr[1w0][1w1];
    }
    @hidden action array5l25_22() {
        hdr.data.arr[1w1][1w0] = hsVar;
    }
    @hidden action array5l25_23() {
        hsiVar_2 = hdr.data.z;
    }
    @hidden action array5l25_24() {
        hdr.data.arr[1w1][1w0] = hdr.data.arr[1w1][1w0];
    }
    @hidden action array5l25_25() {
        hdr.data.arr[1w1][1w0] = hdr.data.arr[1w1][1w1];
    }
    @hidden action array5l25_26() {
        hdr.data.arr[1w1][1w0] = hsVar;
    }
    @hidden action array5l25_27() {
        hsiVar_2 = hdr.data.z;
    }
    @hidden action array5l25_28() {
        hdr.data.arr[1w1][1w0] = hsVar;
    }
    @hidden action array5l25_29() {
        hsiVar_1 = hdr.data.y;
    }
    @hidden action array5l25_30() {
        hdr.data.arr[1w1][1w1] = hdr.data.arr[1w0][1w0];
    }
    @hidden action array5l25_31() {
        hdr.data.arr[1w1][1w1] = hdr.data.arr[1w0][1w1];
    }
    @hidden action array5l25_32() {
        hdr.data.arr[1w1][1w1] = hsVar;
    }
    @hidden action array5l25_33() {
        hsiVar_2 = hdr.data.z;
    }
    @hidden action array5l25_34() {
        hdr.data.arr[1w1][1w1] = hdr.data.arr[1w1][1w0];
    }
    @hidden action array5l25_35() {
        hdr.data.arr[1w1][1w1] = hdr.data.arr[1w1][1w1];
    }
    @hidden action array5l25_36() {
        hdr.data.arr[1w1][1w1] = hsVar;
    }
    @hidden action array5l25_37() {
        hsiVar_2 = hdr.data.z;
    }
    @hidden action array5l25_38() {
        hdr.data.arr[1w1][1w1] = hsVar;
    }
    @hidden action array5l25_39() {
        hsiVar_1 = hdr.data.y;
    }
    @hidden action array5l25_40() {
        hsiVar_0 = hdr.data.x;
    }
    @hidden action array5l25_41() {
        hsiVar = hdr.data.w;
    }
    @hidden table tbl_array5l25 {
        actions = {
            array5l25_41();
        }
        const default_action = array5l25_41();
    }
    @hidden table tbl_array5l25_0 {
        actions = {
            array5l25_19();
        }
        const default_action = array5l25_19();
    }
    @hidden table tbl_array5l25_1 {
        actions = {
            array5l25_8();
        }
        const default_action = array5l25_8();
    }
    @hidden table tbl_array5l25_2 {
        actions = {
            array5l25_2();
        }
        const default_action = array5l25_2();
    }
    @hidden table tbl_array5l25_3 {
        actions = {
            array5l25();
        }
        const default_action = array5l25();
    }
    @hidden table tbl_array5l25_4 {
        actions = {
            array5l25_0();
        }
        const default_action = array5l25_0();
    }
    @hidden table tbl_array5l25_5 {
        actions = {
            array5l25_1();
        }
        const default_action = array5l25_1();
    }
    @hidden table tbl_array5l25_6 {
        actions = {
            array5l25_6();
        }
        const default_action = array5l25_6();
    }
    @hidden table tbl_array5l25_7 {
        actions = {
            array5l25_3();
        }
        const default_action = array5l25_3();
    }
    @hidden table tbl_array5l25_8 {
        actions = {
            array5l25_4();
        }
        const default_action = array5l25_4();
    }
    @hidden table tbl_array5l25_9 {
        actions = {
            array5l25_5();
        }
        const default_action = array5l25_5();
    }
    @hidden table tbl_array5l25_10 {
        actions = {
            array5l25_7();
        }
        const default_action = array5l25_7();
    }
    @hidden table tbl_array5l25_11 {
        actions = {
            array5l25_18();
        }
        const default_action = array5l25_18();
    }
    @hidden table tbl_array5l25_12 {
        actions = {
            array5l25_12();
        }
        const default_action = array5l25_12();
    }
    @hidden table tbl_array5l25_13 {
        actions = {
            array5l25_9();
        }
        const default_action = array5l25_9();
    }
    @hidden table tbl_array5l25_14 {
        actions = {
            array5l25_10();
        }
        const default_action = array5l25_10();
    }
    @hidden table tbl_array5l25_15 {
        actions = {
            array5l25_11();
        }
        const default_action = array5l25_11();
    }
    @hidden table tbl_array5l25_16 {
        actions = {
            array5l25_16();
        }
        const default_action = array5l25_16();
    }
    @hidden table tbl_array5l25_17 {
        actions = {
            array5l25_13();
        }
        const default_action = array5l25_13();
    }
    @hidden table tbl_array5l25_18 {
        actions = {
            array5l25_14();
        }
        const default_action = array5l25_14();
    }
    @hidden table tbl_array5l25_19 {
        actions = {
            array5l25_15();
        }
        const default_action = array5l25_15();
    }
    @hidden table tbl_array5l25_20 {
        actions = {
            array5l25_17();
        }
        const default_action = array5l25_17();
    }
    @hidden table tbl_array5l25_21 {
        actions = {
            array5l25_40();
        }
        const default_action = array5l25_40();
    }
    @hidden table tbl_array5l25_22 {
        actions = {
            array5l25_29();
        }
        const default_action = array5l25_29();
    }
    @hidden table tbl_array5l25_23 {
        actions = {
            array5l25_23();
        }
        const default_action = array5l25_23();
    }
    @hidden table tbl_array5l25_24 {
        actions = {
            array5l25_20();
        }
        const default_action = array5l25_20();
    }
    @hidden table tbl_array5l25_25 {
        actions = {
            array5l25_21();
        }
        const default_action = array5l25_21();
    }
    @hidden table tbl_array5l25_26 {
        actions = {
            array5l25_22();
        }
        const default_action = array5l25_22();
    }
    @hidden table tbl_array5l25_27 {
        actions = {
            array5l25_27();
        }
        const default_action = array5l25_27();
    }
    @hidden table tbl_array5l25_28 {
        actions = {
            array5l25_24();
        }
        const default_action = array5l25_24();
    }
    @hidden table tbl_array5l25_29 {
        actions = {
            array5l25_25();
        }
        const default_action = array5l25_25();
    }
    @hidden table tbl_array5l25_30 {
        actions = {
            array5l25_26();
        }
        const default_action = array5l25_26();
    }
    @hidden table tbl_array5l25_31 {
        actions = {
            array5l25_28();
        }
        const default_action = array5l25_28();
    }
    @hidden table tbl_array5l25_32 {
        actions = {
            array5l25_39();
        }
        const default_action = array5l25_39();
    }
    @hidden table tbl_array5l25_33 {
        actions = {
            array5l25_33();
        }
        const default_action = array5l25_33();
    }
    @hidden table tbl_array5l25_34 {
        actions = {
            array5l25_30();
        }
        const default_action = array5l25_30();
    }
    @hidden table tbl_array5l25_35 {
        actions = {
            array5l25_31();
        }
        const default_action = array5l25_31();
    }
    @hidden table tbl_array5l25_36 {
        actions = {
            array5l25_32();
        }
        const default_action = array5l25_32();
    }
    @hidden table tbl_array5l25_37 {
        actions = {
            array5l25_37();
        }
        const default_action = array5l25_37();
    }
    @hidden table tbl_array5l25_38 {
        actions = {
            array5l25_34();
        }
        const default_action = array5l25_34();
    }
    @hidden table tbl_array5l25_39 {
        actions = {
            array5l25_35();
        }
        const default_action = array5l25_35();
    }
    @hidden table tbl_array5l25_40 {
        actions = {
            array5l25_36();
        }
        const default_action = array5l25_36();
    }
    @hidden table tbl_array5l25_41 {
        actions = {
            array5l25_38();
        }
        const default_action = array5l25_38();
    }
    apply {
        tbl_array5l25.apply();
        if (hsiVar == 1w0) {
            tbl_array5l25_0.apply();
            if (hsiVar_0 == 1w0) {
                tbl_array5l25_1.apply();
                if (hsiVar_1 == 1w0) {
                    tbl_array5l25_2.apply();
                    if (hsiVar_2 == 1w0) {
                        tbl_array5l25_3.apply();
                    } else if (hsiVar_2 == 1w1) {
                        tbl_array5l25_4.apply();
                    } else if (hsiVar_2 >= 1w1) {
                        tbl_array5l25_5.apply();
                    }
                } else if (hsiVar_1 == 1w1) {
                    tbl_array5l25_6.apply();
                    if (hsiVar_2 == 1w0) {
                        tbl_array5l25_7.apply();
                    } else if (hsiVar_2 == 1w1) {
                        tbl_array5l25_8.apply();
                    } else if (hsiVar_2 >= 1w1) {
                        tbl_array5l25_9.apply();
                    }
                } else if (hsiVar_1 >= 1w1) {
                    tbl_array5l25_10.apply();
                }
            } else if (hsiVar_0 == 1w1) {
                tbl_array5l25_11.apply();
                if (hsiVar_1 == 1w0) {
                    tbl_array5l25_12.apply();
                    if (hsiVar_2 == 1w0) {
                        tbl_array5l25_13.apply();
                    } else if (hsiVar_2 == 1w1) {
                        tbl_array5l25_14.apply();
                    } else if (hsiVar_2 >= 1w1) {
                        tbl_array5l25_15.apply();
                    }
                } else if (hsiVar_1 == 1w1) {
                    tbl_array5l25_16.apply();
                    if (hsiVar_2 == 1w0) {
                        tbl_array5l25_17.apply();
                    } else if (hsiVar_2 == 1w1) {
                        tbl_array5l25_18.apply();
                    } else if (hsiVar_2 >= 1w1) {
                        tbl_array5l25_19.apply();
                    }
                } else if (hsiVar_1 >= 1w1) {
                    tbl_array5l25_20.apply();
                }
            }
        } else if (hsiVar == 1w1) {
            tbl_array5l25_21.apply();
            if (hsiVar_0 == 1w0) {
                tbl_array5l25_22.apply();
                if (hsiVar_1 == 1w0) {
                    tbl_array5l25_23.apply();
                    if (hsiVar_2 == 1w0) {
                        tbl_array5l25_24.apply();
                    } else if (hsiVar_2 == 1w1) {
                        tbl_array5l25_25.apply();
                    } else if (hsiVar_2 >= 1w1) {
                        tbl_array5l25_26.apply();
                    }
                } else if (hsiVar_1 == 1w1) {
                    tbl_array5l25_27.apply();
                    if (hsiVar_2 == 1w0) {
                        tbl_array5l25_28.apply();
                    } else if (hsiVar_2 == 1w1) {
                        tbl_array5l25_29.apply();
                    } else if (hsiVar_2 >= 1w1) {
                        tbl_array5l25_30.apply();
                    }
                } else if (hsiVar_1 >= 1w1) {
                    tbl_array5l25_31.apply();
                }
            } else if (hsiVar_0 == 1w1) {
                tbl_array5l25_32.apply();
                if (hsiVar_1 == 1w0) {
                    tbl_array5l25_33.apply();
                    if (hsiVar_2 == 1w0) {
                        tbl_array5l25_34.apply();
                    } else if (hsiVar_2 == 1w1) {
                        tbl_array5l25_35.apply();
                    } else if (hsiVar_2 >= 1w1) {
                        tbl_array5l25_36.apply();
                    }
                } else if (hsiVar_1 == 1w1) {
                    tbl_array5l25_37.apply();
                    if (hsiVar_2 == 1w0) {
                        tbl_array5l25_38.apply();
                    } else if (hsiVar_2 == 1w1) {
                        tbl_array5l25_39.apply();
                    } else if (hsiVar_2 >= 1w1) {
                        tbl_array5l25_40.apply();
                    }
                } else if (hsiVar_1 >= 1w1) {
                    tbl_array5l25_41.apply();
                }
            }
        }
    }
}

top<headers>(c()) main;
