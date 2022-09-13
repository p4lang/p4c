#include <core.p4>

const bit<24> P4CALC_ADD = 24w0x2b;
const bit<24> P4CALC_SUB = 24w0x2d;
const bit<24> P4CALC_MUL = 24w0x2a;
const bit<24> P4CALC_DIV = 24w0x2f;
const bit<24> P4CALC_MOD = 24w0x25;
const bit<24> P4CALC_BAND = 24w0x26;
const bit<24> P4CALC_BOR = 24w0x7c;
const bit<24> P4CALC_BXOR = 24w0x5e;
const bit<24> P4CALC_SHL = 24w0x3c3c;
const bit<24> P4CALC_SHR = 24w0x3e3e;
const bit<24> P4CALC_SATADD = 24w0x7c2b7c;
const bit<24> P4CALC_SATSUB = 24w0x7c2d7c;
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
    action operation_add() {
        bit<32> result = hdr.p4calc.operand_a;
        result = result + hdr.p4calc.operand_b;
        hdr.p4calc.res = result;
    }
    action operation_sub() {
        bit<32> result = hdr.p4calc.operand_a;
        result = result - hdr.p4calc.operand_b;
        hdr.p4calc.res = result;
    }
    action operation_mul() {
        bit<32> result = hdr.p4calc.operand_a;
        result = result * hdr.p4calc.operand_b;
        hdr.p4calc.res = result;
    }
    action operation_div() {
        bit<32> result = hdr.p4calc.operand_a;
        result = result / hdr.p4calc.operand_b;
        hdr.p4calc.res = result;
    }
    action operation_mod() {
        bit<32> result = hdr.p4calc.operand_a;
        result = result % hdr.p4calc.operand_b;
        hdr.p4calc.res = result;
    }
    action operation_band() {
        bit<32> result = hdr.p4calc.operand_a;
        result = result & hdr.p4calc.operand_b;
        hdr.p4calc.res = result;
    }
    action operation_bor() {
        bit<32> result = hdr.p4calc.operand_a;
        result = result | hdr.p4calc.operand_b;
        hdr.p4calc.res = result;
    }
    action operation_bxor() {
        bit<32> result = hdr.p4calc.operand_a;
        result = result ^ hdr.p4calc.operand_b;
        hdr.p4calc.res = result;
    }
    action operation_shl() {
        bit<32> result = hdr.p4calc.operand_a;
        result = result << hdr.p4calc.operand_b;
        hdr.p4calc.res = result;
    }
    action operation_shr() {
        bit<32> result = hdr.p4calc.operand_a;
        result = result >> hdr.p4calc.operand_b;
        hdr.p4calc.res = result;
    }
    action operation_addsat() {
        bit<32> result = hdr.p4calc.operand_a;
        result = result |+| hdr.p4calc.operand_b;
        hdr.p4calc.res = result;
    }
    action operation_subsat() {
        bit<32> result = hdr.p4calc.operand_a;
        result = result |-| hdr.p4calc.operand_b;
        hdr.p4calc.res = result;
    }
    table calculate {
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
            NoAction();
        }
        const default_action = NoAction();
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
            calculate.apply();
        }
    }
}

control Ingress<H>(inout H hdr);
package s<H>(Ingress<H> ig);
s<headers>(caller()) main;
