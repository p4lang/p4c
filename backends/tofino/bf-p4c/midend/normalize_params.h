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

#ifndef _MIDEND_NORMALIZE_PARAMS_H_
#define _MIDEND_NORMALIZE_PARAMS_H_

#include "bf-p4c/midend/type_checker.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/typeMap.h"
#include "ir/ir.h"

/** At the P4 level, the TNA architecture provides an interface for users to
 * define custom parsers/controls/deparsers that are parameterized on
 * architecture-supplied inputs and outputs. For example, a user might supply
 * the following ingress parser and ingress MAU control:
 *
 *  parser ingress_parser(
 *      packet_in pkt,
 *      out my_hdr_t parser_hdr,
 *      out my_meta_t ig_md,
 *      out ingress_intrinsic_metadata_t ig_intr_md);
 *
 *  control ingress_control(
 *      inout my_hdr_t mau_hdr,
 *      inout my_meta_t ig_md,
 *      in ingress_intrinsic_metadata_t ig_intr_md,
 *      in ingress_intrinsic_metadata_from_parser_t ig_intr_md_from_prsr,
 *      inout ingress_intrinsic_metadata_for_deparser_t ig_intr_md_for_dprsr,
 *      inout ingress_intrinsic_metadata_for_tm_t ig_intr_md_for_tm);
 *
 * However, the `parser_hdr` value produced by the parser is actually the same
 * value passed as `mau_hdr` as input to the control.
 *
 *
 * This pass replaces the user-supplied parameter names (eg. `parser_hdr` and
 * `mau_hdr`) with the corresponding parameter names defined in the
 * architecture (`hdr`, in this case).
 *
 * These names are only changed internally; the original names are still
 * exposed to the control plane via the @name annotations attached in the
 * midend.  The one exception is the snapshot mechanism, which will need to
 * refer to the architecture-supplied parameter names when referencing PHV
 * fields.
 *
 * If the user defines the same parameter names as the architecture, then this
 * pass has no effect.  However, if the user defines other instances with the
 * same names as any of the architecture params, this pass raises an error, as
 * those names are reserved.
 */
// TODO: Rather than producing an error, it would be better to rewrite the
// conflicting user-supplied instance names.
class NormalizeParams : public Modifier {
    /// Maps (original) parameter node pointers for each block to the names
    /// that should replace them.
    using Renaming = ordered_map<const IR::Parameter *, cstring>;
    ordered_map<const IR::Node *, Renaming> renaming;

    Modifier::profile_t init_apply(const IR::Node *root) override;
    bool preorder(IR::P4Parser *parser) override;
    bool preorder(IR::P4Control *control) override;

 public:
    explicit NormalizeParams(const IR::ToplevelBlock *) {}
};

/**
 * @ingroup bridged_packing
 * @ingroup parde
 * @brief Pass that governs replacement of the user-supplied parameter names
 *        with the corresponding parameter names defined in the architecture.
 */
class RenameArchParams : public PassManager {
    const IR::ToplevelBlock *toplevel;

 public:
    RenameArchParams(P4::ReferenceMap *refMap, P4::TypeMap *typeMap) {
        auto evaluator = new BFN::EvaluatorPass(refMap, typeMap);
        auto eval =
            new VisitFunctor([this, evaluator]() { toplevel = evaluator->getToplevelBlock(); });
        passes.push_back(eval);
        passes.push_back(new NormalizeParams(toplevel));
    }
};

#endif /* _MIDEND_NORMALIZE_PARAMS_H_ */
