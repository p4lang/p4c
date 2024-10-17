/**
 * Copyright 2013-2024 Intel Corporation.
 *
 * This software and the related documents are Intel copyrighted materials, and your use of them
 * is governed by the express license under which they were provided to you ("License"). Unless
 * the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
 * or transmit this software or the related documents without Intel's prior written permission.
 *
 * This software and the related documents are provided as is, with no express or implied
 * warranties, other than those that are expressly stated in the License.
 */

/**
 * \defgroup midend Midend
 * \brief Overview of midend passes
 *
 * The mid-end performs a mix of passes from p4c and bf-p4c. These passes
 * modify the IR, usually making transformations that keep the functionality
 * of the code, but changes it to be more optimal (removing unnecessary code,
 * moving stuff around, ...). The goal of midend is mostly to transform the IR into
 * a shape that is easily translatable for the backend.
 *
 * The form of mid-end IR is the same as the form of front-end IR.
 * The passes from p4c are enclosed in the P4 namespace while the passes
 * from bf-p4c are enclosed in the BFN namespace.
 *
 * There are two main structures used across most of the passes:
 * * P4::ReferenceMap maps paths found within the IR to a declaration node for
 *   the entity the path refers to.
 * * P4::TypeMap maps nodes of the IR to their respective types.
 *
 * Tied to those structures there are also some general passes that fill them out
 * (that are invoked repeatedly, usually after/before each IR modification) or clear them:
 * * BFN::TypeInference
 * * BFN::TypeChecking
 * * P4::ClearTypeMap - clears the P4::TypeMap if the program has changed or the 'force' flag is set.
 *
 * There are also some best practices that are tied to using BFN::TypeChecking and P4::ReferenceMap/P4::TypeMap:
 * * Each top level PassManager (that will in any way use P4::ReferenceMap or P4::TypeMap) should start by invoking
 *   BFN::TypeChecking to ensure that it is working with the up-to-date data.
 * * BFN::TypeChecking should be also invoked in the middle of a PassManager anytime the IR changes in a way that might
 *   change P4::ReferenceMap or P4::TypeMap (changing indentificators, changing types, ...) and some of the
 *   subsequent passes need those structures to be up-to-date. There might be some
 *   exceptions to this - for example if some of the subsequent passes still require the old maps to update different
 *   parts of the IR correctly. In those cases it should still be invoked after the last such pass is completed.
 * * P4::ClearTypeMap should be used as the last pass of each PassManager (or even before every BFN::TypeChecking
 *   invocation) to force the need to use BFN::TypeChecking in case anything changed.
 *
 * There are also some other helpful passes that are used repeatedly:
 * * BFN::EvaluatorPass evaluates the program and creates blocks for all high-level constucts.
 * * P4::MethodInstance and P4::ExternInstance extract information about different instances
 *   from expressions.
 *
 * Details on some of the passes can be found in the modules and classes sections of this page.
 * The following P4 (parts) passes are used as they are and have no further
 * description yet:
 * * P4::RemoveMiss - replaces miss with not hit.
 * * P4::EliminateNewtype - changes new types into their actual definitions.
 * * P4::EliminateSerEnums - replaces serializable enum constants to the values.
 * * P4::OrderArguments - arguments of a call put into an order in which the parameters appear.
 * * P4::ConvertEnums - converts Type_Enum to Type_Bits.
 * * P4::ConstantFolding - statically evaluates constant expressions.
 * * P4::EliminateTypedef - converts typedef to the actual type it represents.
 * * P4::SimplifyControlFlow - removes empty statements, simplifies if/switch statements.
 * * P4::SimplifyKey - simplifies expressions in keys.
 * * P4::RemoveExits - removes exit statement calls.
 * * P4::StrengthReduction – simplifies expensive arithmetic and boolean operations (determines if
 *   expression is 0/1/true/false/power of 2).
 * * P4::SimplifySelectCases – removes unreachable select cases, removes select with only 1 case.
 * * P4::ExpandLookahead – converts lookahead<T> into lookahead<bit<sizeof(T)>>.
 * * P4::ExpandEmit – converts emit of a header into emits of the header fields.
 * * P4::SimplifyParsers – removes unreachable parser states, collapses simple chains of states.
 * * P4::ReplaceSelectRange – replaces types for select ranges.
 * * P4::EliminateTuples – converts tuples into structures.
 * * P4::SimplifyComparisons – converts comparisons of structures into comparisons of their fields.
 * * P4::NestedStructs – removes nested structures.
 * * P4::SimplifySelectList – removes tuples from select statements.
 * * P4::RemoveSelectBooleans – converts booleans in select statements into 1 bit variables.
 * * P4::Predication – converts if-else statements into "condition ? true : false".
 * * P4::MoveDeclarations – moves local declarations from control or parser to the top.
 * * NormalizeHashList - replace expressions in hash field lists by temporary variables.
 * * P4::SimplifyBitwise - converts modify_field with mask into an bitwise operations.
 * * P4::LocalCopyPropagation – local copy propagation and dead code elimination.
 * * P4::TableHit – converts assignment of hit into if statement.
 * * P4::SynthesizeActions – creates new actions for control assignment statements.
 * * P4::MoveActionsToTables – creates table invocations for direct action calls.
 */

#include "midend.h"

#include "frontends/common/constantFolding.h"
#include "frontends/common/resolveReferences/resolveReferences.h"
#include "frontends/p4/evaluator/evaluator.h"
#include "frontends/p4-14/fromv1.0/v1model.h"
#include "frontends/p4/moveDeclarations.h"
#include "frontends/p4/simplify.h"
#include "frontends/p4/simplifyDefUse.h"
#include "frontends/p4/simplifyParsers.h"
#include "frontends/p4/strengthReduction.h"
#include "frontends/p4/typeMap.h"
#include "frontends/p4/unusedDeclarations.h"
#include "midend/actionSynthesis.h"
#include "midend/compileTimeOps.h"
#include "midend/complexComparison.h"
#include "midend/convertEnums.h"
#include "midend/copyStructures.h"
#include "midend/defuse.h"
#include "midend/eliminateTuples.h"
#include "midend/eliminateNewtype.h"
#include "midend/eliminateSerEnums.h"
#include "midend/eliminateTypedefs.h"
#include "midend/expandEmit.h"
#include "midend/expandLookahead.h"
#include "midend/flattenInterfaceStructs.h"
#include "midend/local_copyprop.h"
#include "midend/nestedStructs.h"
#include "midend/move_to_egress.h"
#include "midend/normalize_hash_list.h"
#include "midend/orderArguments.h"
#include "midend/parser_enforce_depth_req.h"
#include "midend/predication.h"
#include "midend/remove_action_params.h"
#include "midend/removeLeftSlices.h"
#include "midend/removeMiss.h"
#include "midend/removeSelectBooleans.h"
#include "midend/removeExits.h"
#include "midend/replaceSelectRange.h"
#include "midend/simplifyBitwise.h"
#include "midend/simplifyKey.h"
#include "midend/simplifySelectCases.h"
#include "midend/simplifySelectList.h"
#include "midend/alpm.h"
#include "midend/tableHit.h"
#include "midend/validateProperties.h"
#include "bf-p4c/arch/arch.h"
#include "bf-p4c/control-plane/runtime.h"
#include "bf-p4c/midend/action_synthesis_policy.h"
#include "bf-p4c/midend/annotate_with_in_hash.h"
#include "bf-p4c/midend/blockmap.h"
#include "bf-p4c/midend/check_design_pattern.h"
#include "bf-p4c/midend/check_header_alignment.h"
#include "bf-p4c/midend/check_register_actions.h"
#include "bf-p4c/midend/check_unsupported.h"
#include "bf-p4c/midend/copy_block_pragmas.h"
#include "bf-p4c/midend/copy_header.h"
#include "bf-p4c/midend/elim_cast.h"
#include "bf-p4c/midend/eliminate_tuples.h"
#include "bf-p4c/midend/fold_constant_hashes.h"
#include "bf-p4c/midend/desugar_varbit_extract.h"
#include "bf-p4c/midend/drop_packet_with_mirror_engine.h"
#include "bf-p4c/midend/initialize_mirror_io_select.h"
#include "bf-p4c/midend/normalize_params.h"
#include "bf-p4c/midend/ping_pong_generation.h"
#include "bf-p4c/midend/register_read_write.h"
#include "bf-p4c/midend/rewrite_egress_intrinsic_metadata_header.h"
#include "bf-p4c/midend/rewrite_flexible_header.h"
#include "bf-p4c/midend/simplifyIfStatement.h"
#include "bf-p4c/midend/simplify_nested_if.h"
#include "bf-p4c/midend/simplify_args.h"
#include "bf-p4c/midend/type_checker.h"
#include "bf-p4c/midend/simplify_key_policy.h"
#include "bf-p4c/ir/tofino_write_context.h"
#include "bf-p4c/logging/source_info_logging.h"
#include "bf-p4c/midend/remove_select_booleans.h"

namespace BFN {

/**
 * \ingroup midend
 * \brief Pass that converts optional match type to ternary.
 *
 * This class implements a pass to convert optional match type to ternary.
 * Optional is a special case of ternary which allows for 2 cases
 *
 * 1. Is Valid = true , mask = all 1's (Exact Match)
 * 2. Is Valid = false, mask = dont care (Any value)
 *
 * The control plane API does the necessary checks for valid use cases and
 * programs the ternary accordingly.
 *
 * Currently the support exists in PSA & V1Model. But as the pass is common to
 * all archs, in future if TNA needs this simply add 'optional' to tofino.p4
 *
 * BF-RT API Additions:
 * https://wiki.ith.intel.com/display/BXDHOME/BFRT+Optional+match+support
 */
class OptionalToTernaryMatchTypeConverter: public Transform {
 public:
    const IR::Node* postorder(IR::PathExpression *path) {
        auto *key = findContext<IR::KeyElement>();
        if (!key) return path;
        LOG3("OptionalToTernaryMatchTypeConverter postorder : " << path);
        if (path->toString() != "optional") return path;
        return new IR::PathExpression("ternary");
    }
};


/**
 * \ingroup midend
 * \brief Class that implements a policy suitable for the ConvertEnums pass.
 *
 * This class implements a policy suitable for the ConvertEnums pass.
 * The policy is: convert all enums that are not part of the architecture files, and
 * are not used as the output type from a RegisterAction.  These latter enums will get
 * a special encoding later to be compatible with the stateful alu predicate output.
 * Use 32-bit values for all enums.
 */
class EnumOn32Bits : public P4::ChooseEnumRepresentation {
    std::set<cstring> reserved_enums = {
        "MeterType_t"_cs,     "MeterColor_t"_cs, "CounterType_t"_cs, "SelectorMode_t"_cs,
        "HashAlgorithm_t"_cs, "MathOp_t"_cs,     "CloneType"_cs,     "ChecksumAlgorithm_t"_cs};

    bool convert(const IR::Type_Enum* type) const override {
        LOG1("convert ? " << type->name);
        if (reserved_enums.count(type->name))
            return false;
        return true; }

    unsigned enumSize(unsigned) const override { return 32; }

 public:
    /**
     * \brief Pass that creates a policy for ConvertEnums.
     */
    class FindStatefulEnumOutputs : public Inspector {
        EnumOn32Bits &self;
        void postorder(const IR::Declaration_Instance *di) {
            if (auto *ts = di->type->to<IR::Type_Specialized>()) {
                auto bt = ts->baseType->toString();
                unsigned idx = 0;
                if (bt.startsWith("DirectRegisterAction")) {
                    idx = 1;
                } else if (bt.startsWith("RegisterAction")) {
                    idx = 2;
                } else if (bt.startsWith("LearnAction")) {
                    idx = 3;
                } else if (bt.startsWith("MinMaxAction")) {
                    idx = 2;
                } else {
                    return; }
                while (idx < ts->arguments->size()) {
                    auto return_type = ts->arguments->at(idx);
                    if (return_type->is<IR::Type_Name>())
                        self.reserved_enums.insert(return_type->toString());
                    ++idx; }
            }
        }

     public:
        explicit FindStatefulEnumOutputs(EnumOn32Bits &self) : self(self) {}
    };
};

/**
 * \ingroup midend
 *
 * This function implements a policy suitable for the LocalCopyPropagation pass.
 * The policy is: do not local copy propagate for assignment statement
 * setting the output param of a register action.
 * This function returns true if the localCopyPropagation should be enabled;
 * It returns false if the localCopyPropagation should be disabled for the current statement.
 */
template <class T> inline const T *findContext(const Visitor::Context *c) {
    while ((c = c->parent))
        if (auto *rv = dynamic_cast<const T *>(c->node)) return rv;
    return nullptr; }

/**
 * \ingroup midend
 */
bool skipRegisterActionOutput(const Visitor::Context *ctxt, const IR::Expression *) {
    auto c = findContext<IR::Declaration_Instance>(ctxt);
    if (!c) return true;

    auto type = c->type->to<IR::Type_Specialized>();
    if (!type) return true;

    auto name = type->baseType->to<IR::Type_Name>();
    if (!name->path->name.name.endsWith("Action"))
        return true;

    auto f = findContext<IR::Function>(ctxt);
    if (!f) return true;

    auto method = f->type->to<IR::Type_Method>();
    if (!method) return true;

    auto params = method->parameters->to<IR::ParameterList>();
    if (!params) return true;

    auto assign = dynamic_cast<const IR::AssignmentStatement*>(ctxt->parent->node);
    if (!assign) return true;

    auto dest_path = assign->left->to<IR::PathExpression>();
    if (!dest_path) return true;

    for (unsigned i = 1; i < params->size(); ++i)
        if (auto param = params->parameters.at(i)->to<IR::Parameter>())
            if (dest_path->path->name == param->name)
                return false;

    return true;
}

bool skipFlexibleHeader(const Visitor::Context *, const IR::Type_StructLike* e) {
    if (e->getAnnotation("flexible"_cs))
        return false;
    return true;
}

/**
 * \ingroup midend
 * \brief Pass that checks for operations that are defined at compile time (Div, Mod).
 *
 * FIXME -- perhaps just remove this pass altogether and check for unsupported
 * div/mod in instruction selection.
 */
class CompileTimeOperations : public P4::CompileTimeOperations {
    bool preorder(const IR::Declaration_Instance *di) {
#ifdef HAVE_JBAY
        // JBay supports (limited) div/mod in RegisterAction
        if (Device::currentDevice() == Device::JBAY
        ) {
            if (auto st = di->type->to<IR::Type_Specialized>()) {
                if (st->baseType->path->name.name.endsWith("Action"))
                    return false; } }
#endif
        return true;
    }
};

/**
 * \ingroup midend
 * \brief Final midend pass.
 */
class MidEndLast : public PassManager {
 public:
    MidEndLast() { setName("MidEndLast"); }
};

MidEnd::MidEnd(BFN_Options& options) {
    // we may come through this path even if the program is actually a P4 v1.0 program
    setName("MidEnd");
    refMap.setIsV1(true);
    auto typeChecking = new BFN::TypeChecking(&refMap, &typeMap);
    auto typeInference = new BFN::TypeInference(&typeMap, false);
    auto evaluator = new BFN::EvaluatorPass(&refMap, &typeMap);
    auto skip_controls = new std::set<cstring>();
    cstring args_to_skip[] = { "ingress_deparser"_cs, "egress_deparser"_cs};
    auto *enum_policy = new EnumOn32Bits;

    sourceInfoLogging = new CollectSourceInfoLogging(refMap);

    addPasses({
        new P4::RemoveMiss(&typeMap),
        new P4::EliminateNewtype(&typeMap, typeChecking),
        new P4::EliminateSerEnums(&typeMap, typeChecking),
        new BFN::TypeChecking(&refMap, &typeMap, true),
        new BFN::SetDefaultSize(true /* warn */),  // set default table size for tables w/o "size="
        new BFN::CheckUnsupported(&refMap, &typeMap),
        new P4::OrderArguments(&typeMap, typeChecking),
        new BFN::OptionalToTernaryMatchTypeConverter(),
        new BFN::ArchTranslation(&refMap, &typeMap, options),
        new BFN::FindArchitecture(),
        new BFN::TypeChecking(&refMap, &typeMap, true),
        new BFN::CheckDesignPattern(&refMap, &typeMap),  // add checks for p4 design pattern here.
        new BFN::SetDefaultSize(false /* warn */),  //  belt and suspenders, in case of IR mutation
        new BFN::InitializeMirrorIOSelect(&refMap, &typeMap),
        new BFN::DropPacketWithMirrorEngine(&refMap, &typeMap),
        new EnumOn32Bits::FindStatefulEnumOutputs(*enum_policy),
        new P4::ConvertEnums(&typeMap, enum_policy, typeChecking),
        new P4::ConstantFolding(&typeMap, true, typeChecking),
        new P4::EliminateTypedef(&typeMap, typeChecking),
        new P4::SimplifyControlFlow(&typeMap, typeChecking),
        new P4::SimplifyKey(&typeMap,
            BFN::KeyIsSimple::getPolicy(typeMap), typeChecking),
        Device::currentDevice() != Device::TOFINO || options.disable_direct_exit ?
            new P4::RemoveExits(&typeMap, typeChecking) : nullptr,
        new P4::ConstantFolding(&typeMap, true, typeChecking),
        new BFN::ElimCasts(&refMap, &typeMap),
        new BFN::AlpmImplementation(&refMap, &typeMap),
        new BFN::TypeChecking(&refMap, &typeMap, true),
        // has to be early enough for setValid to still not be lowered
        new BFN::CheckVarbitAccess(),
        new P4::StrengthReduction(&typeMap, typeChecking),
        new P4::SimplifySelectCases(&typeMap, true, typeChecking),  // constant keysets
        new P4::ExpandLookahead(&typeMap, typeChecking),
        new P4::ExpandEmit(&typeMap, typeChecking),
        new P4::SimplifyParsers(),
        new P4::ReplaceSelectRange(),
        new P4::StrengthReduction(&typeMap, typeChecking),
        // Eliminate Tuples might need to preserve HashListExpressions
        // Therefore we use our own
        new BFN::EliminateTuples(&refMap, &typeMap, typeChecking, typeInference),
        new P4::SimplifyComparisons(&typeMap, typeChecking),
        new BFN::CopyHeaders(&refMap, &typeMap, typeChecking),
        // must run after copy structure
        new P4::SimplifyIfStatement(&refMap, &typeMap),
        new RewriteFlexibleStruct(&refMap, &typeMap),
        new SimplifyEmitArgs(&refMap, &typeMap),
        new P4::NestedStructs(&typeMap, typeChecking),
        new P4::SimplifySelectList(&typeMap, typeChecking),
        new BFN::RemoveSelectBooleans(&refMap, &typeMap),
        new P4::Predication(),
        new P4::MoveDeclarations(),  // more may have been introduced
        new P4::ConstantFolding(&typeMap, true, typeChecking),
        new P4::SimplifyBitwise(),
        new P4::LocalCopyPropagation(&typeMap, typeChecking, skipRegisterActionOutput),
        new P4::ConstantFolding(&typeMap, true, typeChecking),
        new P4::StrengthReduction(&typeMap, typeChecking),
        new P4::MoveDeclarations(),
        new NormalizeHashList(&refMap, &typeMap, typeChecking),
        new P4::SimplifyNestedIf(&refMap, &typeMap, typeChecking),
        new P4::SimplifyControlFlow(&typeMap, typeChecking),
        new CompileTimeOperations(),
        new P4::TableHit(&typeMap, typeChecking),
#if BAREFOOT_INTERNAL
        new ComputeDefUse,  // otherwise unused; testing for CI coverage
#endif
        evaluator,
        new VisitFunctor([=](const IR::Node *root) -> const IR::Node * {
            auto toplevel = evaluator->getToplevelBlock();
            auto main = toplevel->getMain();
            if (main == nullptr)
                // nothing further to do
                return nullptr;
            for (auto arg : args_to_skip) {
                if (!main->getConstructorParameters()->getDeclByName(arg))
                    continue;
                if (auto a = main->getParameterValue(arg))
                    if (auto ctrl = a->to<IR::ControlBlock>())
                        skip_controls->emplace(ctrl->container->name);
            }
            return root;
        }),
        new P4::SynthesizeActions(&refMap, &typeMap,
                new ActionSynthesisPolicy(skip_controls, &refMap, &typeMap), typeChecking),
        new P4::MoveActionsToTables(&refMap, &typeMap, typeChecking),
        new CopyBlockPragmas(&refMap, &typeMap, typeChecking, {"stage"_cs}),
        (options.egress_intr_md_opt) ?
            new RewriteEgressIntrinsicMetadataHeader(&refMap, &typeMap) : nullptr,
        new DesugarVarbitExtract(&refMap, &typeMap),
        new CheckRegisterActions(&typeMap),
        new PingPongGeneration(&refMap, &typeMap),
        new RegisterReadWrite(&refMap, &typeMap, typeChecking),
        new BFN::AnnotateWithInHash(&refMap, &typeMap, typeChecking),
        new BFN::FoldConstantHashes(&refMap, &typeMap, typeChecking),
        BackendOptions().disable_parse_min_depth_limit &&
                BackendOptions().disable_parse_max_depth_limit
            ? nullptr
            : new BFN::ParserEnforceDepthReq(&refMap, evaluator),

        // Collects source info for logging. Call this after all transformations are complete.
        sourceInfoLogging,

        new MidEndLast,
    });
}

}  // namespace BFN
