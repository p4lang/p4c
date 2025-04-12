/*
 * SPDX-FileCopyrightText: 2026 The P4 Language Consortium
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// A legacy %1$x diagnostic must retain the constant's toString() rendering
// and source location when the argument is an IR::Constant pointer.
const bit<4294967296> tooWide = 0;
