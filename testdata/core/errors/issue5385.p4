/*
 * SPDX-FileCopyrightText: 2026 The P4 Language Consortium
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// Specializing a generic extern with int must be rejected when the resulting
// parameter has a direction: parameters of type int must be directionless.
extern MyCounter<I> {
    MyCounter(bit<32> size);
    void count(in I index);
}
control C();
control c()() {
    MyCounter<int>(1024) counter;
    apply {
        counter.count(1024);
    }
}
package P(C c);
P(c()) main;
