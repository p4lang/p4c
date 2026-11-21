/*
 * SPDX-FileCopyrightText: 2025 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// Ensure that errors cannot be disabled.
@command_line("--Wdisable=type-error")
parser p() {
    state accept {
        transition reject;
    }
}
