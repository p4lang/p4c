/*
 * SPDX-FileCopyrightText: 2026 Abhishek Agarwal
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// A negative slice width is an error. Constant folding used to shift by
// a negative amount for it.

const bool tmp = (1 != 8w2[0 +: -1]);
