/*
 * SPDX-FileCopyrightText: 2026 Pantherun Technologies
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <core.p4>

extern E {
    E();
    void f();
}

control C(out bit<8> output) {
    E() e;
    apply {
        tuple<_, _, _, _>           t0 = { "abc", e, 123, lpm};
        tuple<void, bit<8>>         t1;
        tuple<match_kind, bit<8>>   t2 = { lpm, 1 };
        tuple<int>                  t3 = { 2 };
        tuple<string, bit<8>>       t4 = { "abc", 3 };

        output = t0[2] + t1[1] + (bit<8>) t2[1] + t3[0] + t4[1];
    }
}

control CT(out bit<8> output);
package Top(CT c);
Top(C()) main;
