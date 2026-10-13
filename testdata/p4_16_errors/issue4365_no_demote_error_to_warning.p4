/*
 * SPDX-FileCopyrightText: 2025 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// Ensure that errors cannot be demoted to warnings.
@command_line("--Wwarn=type-error")
parser p() {
    state accept {
        transition reject;
    }
}
