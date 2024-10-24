/**
 * Copyright (C) 2024 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License.  You may obtain a copy
 * of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed
 * under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations under the License.
 *
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BACKENDS_TOFINO_BF_P4C_PHV_PRAGMA_PHV_PRAGMAS_H_
#define BACKENDS_TOFINO_BF_P4C_PHV_PRAGMA_PHV_PRAGMAS_H_

#include "bf-p4c/bf-p4c-options.h"
#include "bf-p4c/phv/pragma/pa_alias.h"
#include "bf-p4c/phv/pragma/pa_atomic.h"
#include "bf-p4c/phv/pragma/pa_byte_pack.h"
#include "bf-p4c/phv/pragma/pa_container_size.h"
#include "bf-p4c/phv/pragma/pa_container_type.h"
#include "bf-p4c/phv/pragma/pa_deparser_zero.h"
#include "bf-p4c/phv/pragma/pa_mutually_exclusive.h"
#include "bf-p4c/phv/pragma/pa_no_init.h"
#include "bf-p4c/phv/pragma/pa_no_overlay.h"
#include "bf-p4c/phv/pragma/pa_no_pack.h"
#include "bf-p4c/phv/pragma/pa_solitary.h"

using namespace P4;

namespace PHV {
namespace pragma {

constexpr const char *NOT_PARSED = "not_parsed";
constexpr const char *NOT_DEPARSED = "not_deparsed";
constexpr const char *DISABLE_DEPARSE_ZERO = "pa_disable_deparse_0_optimization";

}  // namespace pragma

class Pragmas : public PassManager {
 public:
    static bool gressValid(cstring gress);
    static bool determinePipeGressArgs(const IR::Vector<IR::Expression> &exprs,
                                       unsigned &expr_index, unsigned &required_args,
                                       const IR::StringLiteral *&pipe_arg,
                                       const IR::StringLiteral *&gress_arg);
    static bool checkStringLiteralArgs(const IR::Vector<IR::Expression> &exprs);
    static bool checkNumberArgs(const IR::Annotation *annotation, unsigned required_args,
                                const unsigned min_required_args, bool exact_number_of_args,
                                cstring pragma_name, cstring pragma_args_wo_pipe);
    static bool checkPipeApplication(const IR::Annotation *annotation, const IR::BFN::Pipe *pipe,
                                     const IR::StringLiteral *pipe_arg);
    static void reportNoMatchingPHV(const IR::BFN::Pipe *pipe, const IR::Expression *expr,
                                    cstring field_name = ""_cs);

 private:
    PragmaContainerSize pa_container_sizes_i;
    PragmaMutuallyExclusive pa_mutually_exclusive_i;
    PragmaSolitary pa_solitary_i;
    PragmaAtomic pa_atomic_i;
    PragmaNoOverlay pa_no_overlay_i;
    PragmaContainerType pa_container_type_i;
    PragmaNoInit pa_no_init_i;
    PragmaDeparserZero pa_deparser_zero_i;
    PragmaNoPack pa_no_pack_i;
    PragmaBytePack pa_byte_pack_i;

 public:
    const PragmaContainerSize &pa_container_sizes() const { return pa_container_sizes_i; }
    PragmaContainerSize &pa_container_sizes() { return pa_container_sizes_i; }

    const PragmaMutuallyExclusive &pa_mutually_exclusive() const { return pa_mutually_exclusive_i; }
    PragmaMutuallyExclusive &pa_mutually_exclusive() { return pa_mutually_exclusive_i; }

    const PragmaSolitary &pa_solitary() const { return pa_solitary_i; }
    PragmaSolitary &pa_solitary() { return pa_solitary_i; }

    const PragmaAtomic &pa_atomic() const { return pa_atomic_i; }
    PragmaAtomic &pa_atomic() { return pa_atomic_i; }

    const PragmaNoOverlay &pa_no_overlay() const { return pa_no_overlay_i; }
    PragmaNoOverlay &pa_no_overlay() { return pa_no_overlay_i; }

    const PragmaContainerType &pa_container_type() const { return pa_container_type_i; }
    PragmaContainerType &pa_container_type() { return pa_container_type_i; }

    const PragmaNoInit &pa_no_init() const { return pa_no_init_i; }
    PragmaNoInit &pa_no_init() { return pa_no_init_i; }

    const PragmaDeparserZero &pa_deparser_zero() const { return pa_deparser_zero_i; }
    PragmaDeparserZero &pa_deparser_zero() { return pa_deparser_zero_i; }

    const PragmaNoPack &pa_no_pack() const { return pa_no_pack_i; }
    PragmaNoPack &pa_no_pack() { return pa_no_pack_i; }

    const PragmaBytePack &pa_byte_pack() const { return pa_byte_pack_i; }
    PragmaBytePack &pa_byte_pack() { return pa_byte_pack_i; }

    explicit Pragmas(PhvInfo &phv)
        : pa_container_sizes_i(phv, *(new ordered_map<cstring, std::vector<PHV::Size>>)),
          pa_mutually_exclusive_i(phv),
          pa_solitary_i(phv),
          pa_atomic_i(phv),
          pa_no_overlay_i(phv),
          pa_container_type_i(phv),
          pa_no_init_i(phv),
          pa_deparser_zero_i(phv),
          pa_no_pack_i(phv),
          pa_byte_pack_i(phv) {
        addPasses({
            &pa_container_sizes_i,
            &pa_mutually_exclusive_i,
            &pa_solitary_i,
            &pa_atomic_i,
            &pa_no_overlay_i,
            &pa_container_type_i,
            &pa_no_init_i,
            &pa_deparser_zero_i,
            &pa_no_pack_i,
            &pa_byte_pack_i,
        });
    }
    Pragmas(PhvInfo &phv, const ordered_map<cstring, std::vector<PHV::Size>> &container_size_constr,
            const ordered_map<cstring, ordered_set<cstring>> &no_pack_constr)
        : pa_container_sizes_i(phv, container_size_constr),
          pa_mutually_exclusive_i(phv),
          pa_solitary_i(phv),
          pa_atomic_i(phv),
          pa_no_overlay_i(phv),
          pa_container_type_i(phv),
          pa_no_init_i(phv),
          pa_deparser_zero_i(phv),
          pa_no_pack_i(phv, no_pack_constr),
          pa_byte_pack_i(phv) {
        addPasses({
            &pa_container_sizes_i,
            &pa_mutually_exclusive_i,
            &pa_solitary_i,
            &pa_atomic_i,
            &pa_no_overlay_i,
            &pa_container_type_i,
            &pa_no_init_i,
            &pa_deparser_zero_i,
            &pa_no_pack_i,
            &pa_byte_pack_i,
        });
    }
};

}  // namespace PHV

#endif /* BACKENDS_TOFINO_BF_P4C_PHV_PRAGMA_PHV_PRAGMAS_H_ */
