// SPDX-FileCopyrightText: 2026 sanket-jadhav-cse
//
// SPDX-License-Identifier: Apache-2.0

enum bit<8> E { A = 1, B = 2 }

parser P(in bit<8> x) {
    state start {
        transition select(E.B, x) {
            (E.A .. true, 7): accept;
            (E.B &&& false, 8): accept;
            default: reject;
        }
    }
}
