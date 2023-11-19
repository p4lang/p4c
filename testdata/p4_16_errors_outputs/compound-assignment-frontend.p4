#include <core.p4>

header p4calc_t {
    bit<24> op;
    bit<32> operand_a;
    bit<32> operand_b;
    bit<32> res;
}

struct headers {
    p4calc_t p4calc;
}

control caller(inout headers hdr) {
    @name("caller.result") bit<32> result_0;
    @name("caller.result") bit<32> result_1;
    @name("caller.result") bit<32> result_2;
    @name("caller.result") bit<32> result_3;
    @name("caller.result") bit<32> result_4;
    @name("caller.result") bit<32> result_5;
    @name("caller.result") bit<32> result_6;
    @name("caller.result") bit<32> result_7;
    @name("caller.result") bit<32> result_8;
    @name("caller.result") bit<32> result_9;
    @name("caller.result") bit<32> result_10;
    @name("caller.result") bit<32> result_11;
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("caller.operation_add") action operation_add() {
        result_0 = hdr.p4calc.operand_a;
        result_0 = result_0 + hdr.p4calc.operand_b;
        hdr.p4calc.res = result_0;
    }
    @name("caller.operation_sub") action operation_sub() {
        result_1 = hdr.p4calc.operand_a;
        result_1 = result_1 - hdr.p4calc.operand_b;
        hdr.p4calc.res = result_1;
    }
    @name("caller.operation_mul") action operation_mul() {
        result_2 = hdr.p4calc.operand_a;
        result_2 = result_2 * hdr.p4calc.operand_b;
        hdr.p4calc.res = result_2;
    }
    @name("caller.operation_div") action operation_div() {
        result_3 = hdr.p4calc.operand_a;
        result_3 = result_3 / hdr.p4calc.operand_b;
        hdr.p4calc.res = result_3;
    }
    @name("caller.operation_mod") action operation_mod() {
        result_4 = hdr.p4calc.operand_a;
        result_4 = result_4 % hdr.p4calc.operand_b;
        hdr.p4calc.res = result_4;
    }
    @name("caller.operation_band") action operation_band() {
        result_5 = hdr.p4calc.operand_a;
        result_5 = result_5 & hdr.p4calc.operand_b;
        hdr.p4calc.res = result_5;
    }
    @name("caller.operation_bor") action operation_bor() {
        result_6 = hdr.p4calc.operand_a;
        result_6 = result_6 | hdr.p4calc.operand_b;
        hdr.p4calc.res = result_6;
    }
    @name("caller.operation_bxor") action operation_bxor() {
        result_7 = hdr.p4calc.operand_a;
        result_7 = result_7 ^ hdr.p4calc.operand_b;
        hdr.p4calc.res = result_7;
    }
    @name("caller.operation_shl") action operation_shl() {
        result_8 = hdr.p4calc.operand_a;
        result_8 = result_8 << hdr.p4calc.operand_b;
        hdr.p4calc.res = result_8;
    }
    @name("caller.operation_shr") action operation_shr() {
        result_9 = hdr.p4calc.operand_a;
        result_9 = result_9 >> hdr.p4calc.operand_b;
        hdr.p4calc.res = result_9;
    }
    @name("caller.operation_addsat") action operation_addsat() {
        result_10 = hdr.p4calc.operand_a;
        result_10 = result_10 |+| hdr.p4calc.operand_b;
        hdr.p4calc.res = result_10;
    }
    @name("caller.operation_subsat") action operation_subsat() {
        result_11 = hdr.p4calc.operand_a;
        result_11 = result_11 |-| hdr.p4calc.operand_b;
        hdr.p4calc.res = result_11;
    }
    @name("caller.calculate") table calculate_0 {
        key = {
            hdr.p4calc.op: exact @name("hdr.p4calc.op");
        }
        actions = {
            operation_add();
            operation_sub();
            operation_mul();
            operation_div();
            operation_mod();
            operation_band();
            operation_bor();
            operation_bxor();
            operation_shl();
            operation_shr();
            operation_addsat();
            operation_subsat();
            NoAction_1();
        }
        const default_action = NoAction_1();
        const entries = {
                        24w0x2b : operation_add();
                        24w0x2d : operation_sub();
                        24w0x2a : operation_mul();
                        24w0x2f : operation_div();
                        24w0x25 : operation_mod();
                        24w0x26 : operation_band();
                        24w0x7c : operation_bor();
                        24w0x5e : operation_bxor();
                        24w0x3c3c : operation_shl();
                        24w0x3e3e : operation_shr();
                        24w0x7c2b7c : operation_addsat();
                        24w0x7c2d7c : operation_subsat();
        }
    }
    apply {
        if (hdr.p4calc.isValid()) {
            calculate_0.apply();
        }
    }
}

control Ingress<H>(inout H hdr);
package s<H>(Ingress<H> ig);
s<headers>(caller()) main;
