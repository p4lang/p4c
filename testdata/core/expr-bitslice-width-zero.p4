/*
 * SPDX-FileCopyrightText: 2026 Abhishek Agarwal
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// A zero width slice x[i +: 0] is legal and has the single value of
// type bit<0>. It used to produce a malformed IR::Slice, issue 5734.

const bool tmp1 = (8w2[0+:0] == 0);
const bool tmp2 = (8w2[3+:0] == 0);

control c(inout bit<8> x) {
    apply {
        if (x[2+:0] == 0) {
            x = x | 1;
        }
    }
}
