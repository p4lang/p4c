/*
 * SPDX-FileCopyrightText: 2026 The P4 Language Consortium
 * SPDX-License-Identifier: Apache-2.0
 */

#include <core.p4>

// System-only error and match_kind declarations must not add blank lines.
control C();
control CImpl() {
    apply {
    }
}

package P(C c);
P(CImpl()) main;
