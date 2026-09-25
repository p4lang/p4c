/*
 * SPDX-FileCopyrightText: 2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// should not compile -- not valid string op
@deprecated("a" | "b")
void fn() {
}
