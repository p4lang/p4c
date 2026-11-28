/*
 * SPDX-FileCopyrightText: 2026 Devansh
 *
 * SPDX-License-Identifier: Apache-2.0
 */

header H<T> {
    T f;
}

control c() {
    apply {
        H<UndefinedType> h;
    }
}