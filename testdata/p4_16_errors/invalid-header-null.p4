/*
 * SPDX-FileCopyrightText: 2026 The P4 Language Consortium & Devansh Singh
 *
 * SPDX-License-Identifier: Apache-2.0
 */

header H<T> {
    T f;
}

control c() {
    apply {
        H<UndefinedType> h = (H<UndefinedType>){#};
    }
}
