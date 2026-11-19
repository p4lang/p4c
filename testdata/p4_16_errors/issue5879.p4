/*
 * SPDX-FileCopyrightText: 2026 Pantherun Technologies
 *
 * SPDX-License-Identifier: Apache-2.0
 */

header H {
    bit<8> f;
}

typedef H<bit<8>> Bad;
type Bad New;
