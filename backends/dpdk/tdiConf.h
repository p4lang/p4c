/*
 * Copyright 2023 Intel Corporation
 * SPDX-FileCopyrightText: 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BACKENDS_DPDK_TDICONF_H_
#define BACKENDS_DPDK_TDICONF_H_

#include <string>

#include "backends/dpdk/options.h"
namespace P4::DPDK {

class TdiBfrtConf {
 private:
    /// Iterates over the arguments of the main declaration instance and returns the callee names as
    /// a vector of cstrings.
    static std::vector<cstring> getPipeNames(const IR::Declaration_Instance *main);

    /// @returns the canonical pipe name for the respective architecture.
    static std::optional<cstring> findPipeName(const IR::P4Program *prog,
                                               DPDK::DpdkOptions &options);

 public:
    /// Generate the TDI configuration file required by the tdi-pipeline-builder.
    static void generate(const IR::P4Program *prog, DPDK::DpdkOptions &options);
};

}  // namespace P4::DPDK

#endif /* BACKENDS_DPDK_TDICONF_H_ */
