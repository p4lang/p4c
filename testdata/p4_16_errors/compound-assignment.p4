/*
Copyright 2022-present University of Oxford

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include <core.p4>

const bit<24>  P4CALC_ADD      = 0x2b;      // '+'
const bit<24>  P4CALC_SUB      = 0x2d;      // '-'
const bit<24>  P4CALC_MUL      = 0x2a;      // '*'
const bit<24>  P4CALC_DIV      = 0x2f;      // '/'
const bit<24>  P4CALC_MOD      = 0x25;      // '%'
const bit<24>  P4CALC_BAND     = 0x26;      // '&'
const bit<24>  P4CALC_BOR      = 0x7c;      // '|'
const bit<24>  P4CALC_BXOR     = 0x5e;      // '^'
const bit<24>  P4CALC_SHL      = 0x3c3c;    // '<<'
const bit<24>  P4CALC_SHR      = 0x3e3e;    // '>>'
const bit<24>  P4CALC_SATADD   = 0x7c2b7c;  // '|+|'
const bit<24>  P4CALC_SATSUB   = 0x7c2d7c;  // '|-|'

header p4calc_t {
    bit<24>  op;
    bit<32> operand_a;
    bit<32> operand_b;
    bit<32> res;
}

struct headers {
    p4calc_t     p4calc;
}

control caller(inout headers hdr) {
    action operation_add() {
        bit<32> result = hdr.p4calc.operand_a;
        result += hdr.p4calc.operand_b;
        hdr.p4calc.res = result;
    }

    action operation_sub() {
        bit<32> result = hdr.p4calc.operand_a;
        result -= hdr.p4calc.operand_b;
        hdr.p4calc.res = result;
    }

    action operation_mul() {
        bit<32> result = hdr.p4calc.operand_a;
        result *= hdr.p4calc.operand_b;
        hdr.p4calc.res = result;
    }

    action operation_div() {
        bit<32> result = hdr.p4calc.operand_a;
        result /= hdr.p4calc.operand_b;
        hdr.p4calc.res = result;
    }

    action operation_mod() {
        bit<32> result = hdr.p4calc.operand_a;
        result %= hdr.p4calc.operand_b;
        hdr.p4calc.res = result;
    }

    action operation_band() {
        bit<32> result = hdr.p4calc.operand_a;
        result &= hdr.p4calc.operand_b;
        hdr.p4calc.res = result;
    }

    action operation_bor() {
        bit<32> result = hdr.p4calc.operand_a;
        result |= hdr.p4calc.operand_b;
        hdr.p4calc.res = result;
    }

    action operation_bxor() {
        bit<32> result = hdr.p4calc.operand_a;
        result ^= hdr.p4calc.operand_b;
        hdr.p4calc.res = result;
    }

    action operation_shl() {
        bit<32> result = hdr.p4calc.operand_a;
        result <<= hdr.p4calc.operand_b;
        hdr.p4calc.res = result;
    }

    action operation_shr() {
        bit<32> result = hdr.p4calc.operand_a;
        result >>= hdr.p4calc.operand_b;
        hdr.p4calc.res = result;
    }

    action operation_addsat() {
        bit<32> result = hdr.p4calc.operand_a;
        result |+|= hdr.p4calc.operand_b;
        hdr.p4calc.res = result;
    }

    action operation_subsat() {
        bit<32> result = hdr.p4calc.operand_a;
        result |-|= hdr.p4calc.operand_b;
        hdr.p4calc.res = result;
    }

    table calculate {
        key = {
            hdr.p4calc.op        : exact;
        }
        actions = {
            operation_add;
            operation_sub;
            operation_mul;
            operation_div;
            operation_mod;
            operation_band;
            operation_bor;
            operation_bxor;
            operation_shl;
            operation_shr;
            operation_addsat;
            operation_subsat;
            NoAction;
        }
        const default_action = NoAction();
        const entries = {
            P4CALC_ADD    : operation_add();
            P4CALC_SUB    : operation_sub();
            P4CALC_MUL    : operation_mul();
            P4CALC_DIV    : operation_div();
            P4CALC_MOD    : operation_mod();
            P4CALC_BAND   : operation_band();
            P4CALC_BOR    : operation_bor();
            P4CALC_BXOR   : operation_bxor();
            P4CALC_SHL    : operation_shl();
            P4CALC_SHR    : operation_shr();
            P4CALC_SATADD : operation_addsat();
            P4CALC_SATSUB : operation_subsat();
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

s(caller()) main;