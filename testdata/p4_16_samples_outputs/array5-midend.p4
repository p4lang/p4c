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
    @hidden action array5l23() {
        hdr.data.arr[1w0][1w0] = hdr.data.arr[1w0][1w0];
    }
    @hidden action array5l23_0() {
        hdr.data.arr[1w0][1w0] = hdr.data.arr[1w0][1w1];
    }
    @hidden action array5l23_1() {
        hdr.data.arr[1w0][1w0] = hsVar;
    }
    @hidden action array5l23_2() {
        hsiVar_2 = hdr.data.z;
    }
    @hidden action array5l23_3() {
        hdr.data.arr[1w0][1w0] = hdr.data.arr[1w1][1w0];
    }
    @hidden action array5l23_4() {
        hdr.data.arr[1w0][1w0] = hdr.data.arr[1w1][1w1];
    }
    @hidden action array5l23_5() {
        hdr.data.arr[1w0][1w0] = hsVar;
    }
    @hidden action array5l23_6() {
        hsiVar_2 = hdr.data.z;
    }
    @hidden action array5l23_7() {
        hdr.data.arr[1w0][1w0] = hsVar;
    }
    @hidden action array5l23_8() {
        hsiVar_1 = hdr.data.y;
    }
    @hidden action array5l23_9() {
        hdr.data.arr[1w0][1w1] = hdr.data.arr[1w0][1w0];
    }
    @hidden action array5l23_10() {
        hdr.data.arr[1w0][1w1] = hdr.data.arr[1w0][1w1];
    }
    @hidden action array5l23_11() {
        hdr.data.arr[1w0][1w1] = hsVar;
    }
    @hidden action array5l23_12() {
        hsiVar_2 = hdr.data.z;
    }
    @hidden action array5l23_13() {
        hdr.data.arr[1w0][1w1] = hdr.data.arr[1w1][1w0];
    }
    @hidden action array5l23_14() {
        hdr.data.arr[1w0][1w1] = hdr.data.arr[1w1][1w1];
    }
    @hidden action array5l23_15() {
        hdr.data.arr[1w0][1w1] = hsVar;
    }
    @hidden action array5l23_16() {
        hsiVar_2 = hdr.data.z;
    }
    @hidden action array5l23_17() {
        hdr.data.arr[1w0][1w1] = hsVar;
    }
    @hidden action array5l23_18() {
        hsiVar_1 = hdr.data.y;
    }
    @hidden action array5l23_19() {
        hsiVar_0 = hdr.data.x;
    }
    @hidden action array5l23_20() {
        hdr.data.arr[1w1][1w0] = hdr.data.arr[1w0][1w0];
    }
    @hidden action array5l23_21() {
        hdr.data.arr[1w1][1w0] = hdr.data.arr[1w0][1w1];
    }
    @hidden action array5l23_22() {
        hdr.data.arr[1w1][1w0] = hsVar;
    }
    @hidden action array5l23_23() {
        hsiVar_2 = hdr.data.z;
    }
    @hidden action array5l23_24() {
        hdr.data.arr[1w1][1w0] = hdr.data.arr[1w1][1w0];
    }
    @hidden action array5l23_25() {
        hdr.data.arr[1w1][1w0] = hdr.data.arr[1w1][1w1];
    }
    @hidden action array5l23_26() {
        hdr.data.arr[1w1][1w0] = hsVar;
    }
    @hidden action array5l23_27() {
        hsiVar_2 = hdr.data.z;
    }
    @hidden action array5l23_28() {
        hdr.data.arr[1w1][1w0] = hsVar;
    }
    @hidden action array5l23_29() {
        hsiVar_1 = hdr.data.y;
    }
    @hidden action array5l23_30() {
        hdr.data.arr[1w1][1w1] = hdr.data.arr[1w0][1w0];
    }
    @hidden action array5l23_31() {
        hdr.data.arr[1w1][1w1] = hdr.data.arr[1w0][1w1];
    }
    @hidden action array5l23_32() {
        hdr.data.arr[1w1][1w1] = hsVar;
    }
    @hidden action array5l23_33() {
        hsiVar_2 = hdr.data.z;
    }
    @hidden action array5l23_34() {
        hdr.data.arr[1w1][1w1] = hdr.data.arr[1w1][1w0];
    }
    @hidden action array5l23_35() {
        hdr.data.arr[1w1][1w1] = hdr.data.arr[1w1][1w1];
    }
    @hidden action array5l23_36() {
        hdr.data.arr[1w1][1w1] = hsVar;
    }
    @hidden action array5l23_37() {
        hsiVar_2 = hdr.data.z;
    }
    @hidden action array5l23_38() {
        hdr.data.arr[1w1][1w1] = hsVar;
    }
    @hidden action array5l23_39() {
        hsiVar_1 = hdr.data.y;
    }
    @hidden action array5l23_40() {
        hsiVar_0 = hdr.data.x;
    }
    @hidden action array5l23_41() {
        hsiVar = hdr.data.w;
    }
    @hidden table tbl_array5l23 {
        actions = {
            array5l23_41();
        }
        const default_action = array5l23_41();
    }
    @hidden table tbl_array5l23_0 {
        actions = {
            array5l23_19();
        }
        const default_action = array5l23_19();
    }
    @hidden table tbl_array5l23_1 {
        actions = {
            array5l23_8();
        }
        const default_action = array5l23_8();
    }
    @hidden table tbl_array5l23_2 {
        actions = {
            array5l23_2();
        }
        const default_action = array5l23_2();
    }
    @hidden table tbl_array5l23_3 {
        actions = {
            array5l23();
        }
        const default_action = array5l23();
    }
    @hidden table tbl_array5l23_4 {
        actions = {
            array5l23_0();
        }
        const default_action = array5l23_0();
    }
    @hidden table tbl_array5l23_5 {
        actions = {
            array5l23_1();
        }
        const default_action = array5l23_1();
    }
    @hidden table tbl_array5l23_6 {
        actions = {
            array5l23_6();
        }
        const default_action = array5l23_6();
    }
    @hidden table tbl_array5l23_7 {
        actions = {
            array5l23_3();
        }
        const default_action = array5l23_3();
    }
    @hidden table tbl_array5l23_8 {
        actions = {
            array5l23_4();
        }
        const default_action = array5l23_4();
    }
    @hidden table tbl_array5l23_9 {
        actions = {
            array5l23_5();
        }
        const default_action = array5l23_5();
    }
    @hidden table tbl_array5l23_10 {
        actions = {
            array5l23_7();
        }
        const default_action = array5l23_7();
    }
    @hidden table tbl_array5l23_11 {
        actions = {
            array5l23_18();
        }
        const default_action = array5l23_18();
    }
    @hidden table tbl_array5l23_12 {
        actions = {
            array5l23_12();
        }
        const default_action = array5l23_12();
    }
    @hidden table tbl_array5l23_13 {
        actions = {
            array5l23_9();
        }
        const default_action = array5l23_9();
    }
    @hidden table tbl_array5l23_14 {
        actions = {
            array5l23_10();
        }
        const default_action = array5l23_10();
    }
    @hidden table tbl_array5l23_15 {
        actions = {
            array5l23_11();
        }
        const default_action = array5l23_11();
    }
    @hidden table tbl_array5l23_16 {
        actions = {
            array5l23_16();
        }
        const default_action = array5l23_16();
    }
    @hidden table tbl_array5l23_17 {
        actions = {
            array5l23_13();
        }
        const default_action = array5l23_13();
    }
    @hidden table tbl_array5l23_18 {
        actions = {
            array5l23_14();
        }
        const default_action = array5l23_14();
    }
    @hidden table tbl_array5l23_19 {
        actions = {
            array5l23_15();
        }
        const default_action = array5l23_15();
    }
    @hidden table tbl_array5l23_20 {
        actions = {
            array5l23_17();
        }
        const default_action = array5l23_17();
    }
    @hidden table tbl_array5l23_21 {
        actions = {
            array5l23_40();
        }
        const default_action = array5l23_40();
    }
    @hidden table tbl_array5l23_22 {
        actions = {
            array5l23_29();
        }
        const default_action = array5l23_29();
    }
    @hidden table tbl_array5l23_23 {
        actions = {
            array5l23_23();
        }
        const default_action = array5l23_23();
    }
    @hidden table tbl_array5l23_24 {
        actions = {
            array5l23_20();
        }
        const default_action = array5l23_20();
    }
    @hidden table tbl_array5l23_25 {
        actions = {
            array5l23_21();
        }
        const default_action = array5l23_21();
    }
    @hidden table tbl_array5l23_26 {
        actions = {
            array5l23_22();
        }
        const default_action = array5l23_22();
    }
    @hidden table tbl_array5l23_27 {
        actions = {
            array5l23_27();
        }
        const default_action = array5l23_27();
    }
    @hidden table tbl_array5l23_28 {
        actions = {
            array5l23_24();
        }
        const default_action = array5l23_24();
    }
    @hidden table tbl_array5l23_29 {
        actions = {
            array5l23_25();
        }
        const default_action = array5l23_25();
    }
    @hidden table tbl_array5l23_30 {
        actions = {
            array5l23_26();
        }
        const default_action = array5l23_26();
    }
    @hidden table tbl_array5l23_31 {
        actions = {
            array5l23_28();
        }
        const default_action = array5l23_28();
    }
    @hidden table tbl_array5l23_32 {
        actions = {
            array5l23_39();
        }
        const default_action = array5l23_39();
    }
    @hidden table tbl_array5l23_33 {
        actions = {
            array5l23_33();
        }
        const default_action = array5l23_33();
    }
    @hidden table tbl_array5l23_34 {
        actions = {
            array5l23_30();
        }
        const default_action = array5l23_30();
    }
    @hidden table tbl_array5l23_35 {
        actions = {
            array5l23_31();
        }
        const default_action = array5l23_31();
    }
    @hidden table tbl_array5l23_36 {
        actions = {
            array5l23_32();
        }
        const default_action = array5l23_32();
    }
    @hidden table tbl_array5l23_37 {
        actions = {
            array5l23_37();
        }
        const default_action = array5l23_37();
    }
    @hidden table tbl_array5l23_38 {
        actions = {
            array5l23_34();
        }
        const default_action = array5l23_34();
    }
    @hidden table tbl_array5l23_39 {
        actions = {
            array5l23_35();
        }
        const default_action = array5l23_35();
    }
    @hidden table tbl_array5l23_40 {
        actions = {
            array5l23_36();
        }
        const default_action = array5l23_36();
    }
    @hidden table tbl_array5l23_41 {
        actions = {
            array5l23_38();
        }
        const default_action = array5l23_38();
    }
    apply {
        tbl_array5l23.apply();
        if (hsiVar == 1w0) {
            tbl_array5l23_0.apply();
            if (hsiVar_0 == 1w0) {
                tbl_array5l23_1.apply();
                if (hsiVar_1 == 1w0) {
                    tbl_array5l23_2.apply();
                    if (hsiVar_2 == 1w0) {
                        tbl_array5l23_3.apply();
                    } else if (hsiVar_2 == 1w1) {
                        tbl_array5l23_4.apply();
                    } else if (hsiVar_2 >= 1w1) {
                        tbl_array5l23_5.apply();
                    }
                } else if (hsiVar_1 == 1w1) {
                    tbl_array5l23_6.apply();
                    if (hsiVar_2 == 1w0) {
                        tbl_array5l23_7.apply();
                    } else if (hsiVar_2 == 1w1) {
                        tbl_array5l23_8.apply();
                    } else if (hsiVar_2 >= 1w1) {
                        tbl_array5l23_9.apply();
                    }
                } else if (hsiVar_1 >= 1w1) {
                    tbl_array5l23_10.apply();
                }
            } else if (hsiVar_0 == 1w1) {
                tbl_array5l23_11.apply();
                if (hsiVar_1 == 1w0) {
                    tbl_array5l23_12.apply();
                    if (hsiVar_2 == 1w0) {
                        tbl_array5l23_13.apply();
                    } else if (hsiVar_2 == 1w1) {
                        tbl_array5l23_14.apply();
                    } else if (hsiVar_2 >= 1w1) {
                        tbl_array5l23_15.apply();
                    }
                } else if (hsiVar_1 == 1w1) {
                    tbl_array5l23_16.apply();
                    if (hsiVar_2 == 1w0) {
                        tbl_array5l23_17.apply();
                    } else if (hsiVar_2 == 1w1) {
                        tbl_array5l23_18.apply();
                    } else if (hsiVar_2 >= 1w1) {
                        tbl_array5l23_19.apply();
                    }
                } else if (hsiVar_1 >= 1w1) {
                    tbl_array5l23_20.apply();
                }
            }
        } else if (hsiVar == 1w1) {
            tbl_array5l23_21.apply();
            if (hsiVar_0 == 1w0) {
                tbl_array5l23_22.apply();
                if (hsiVar_1 == 1w0) {
                    tbl_array5l23_23.apply();
                    if (hsiVar_2 == 1w0) {
                        tbl_array5l23_24.apply();
                    } else if (hsiVar_2 == 1w1) {
                        tbl_array5l23_25.apply();
                    } else if (hsiVar_2 >= 1w1) {
                        tbl_array5l23_26.apply();
                    }
                } else if (hsiVar_1 == 1w1) {
                    tbl_array5l23_27.apply();
                    if (hsiVar_2 == 1w0) {
                        tbl_array5l23_28.apply();
                    } else if (hsiVar_2 == 1w1) {
                        tbl_array5l23_29.apply();
                    } else if (hsiVar_2 >= 1w1) {
                        tbl_array5l23_30.apply();
                    }
                } else if (hsiVar_1 >= 1w1) {
                    tbl_array5l23_31.apply();
                }
            } else if (hsiVar_0 == 1w1) {
                tbl_array5l23_32.apply();
                if (hsiVar_1 == 1w0) {
                    tbl_array5l23_33.apply();
                    if (hsiVar_2 == 1w0) {
                        tbl_array5l23_34.apply();
                    } else if (hsiVar_2 == 1w1) {
                        tbl_array5l23_35.apply();
                    } else if (hsiVar_2 >= 1w1) {
                        tbl_array5l23_36.apply();
                    }
                } else if (hsiVar_1 == 1w1) {
                    tbl_array5l23_37.apply();
                    if (hsiVar_2 == 1w0) {
                        tbl_array5l23_38.apply();
                    } else if (hsiVar_2 == 1w1) {
                        tbl_array5l23_39.apply();
                    } else if (hsiVar_2 >= 1w1) {
                        tbl_array5l23_40.apply();
                    }
                } else if (hsiVar_1 >= 1w1) {
                    tbl_array5l23_41.apply();
                }
            }
        }
    }
}

top<headers>(c()) main;
