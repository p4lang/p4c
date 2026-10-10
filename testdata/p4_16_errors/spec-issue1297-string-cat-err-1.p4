/*
 * SPDX-FileCopyrightText: 2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

extern void log(string msg);

void fn() {
    log("a" ++ 4);
    log(1w2 ++ "a");
}
