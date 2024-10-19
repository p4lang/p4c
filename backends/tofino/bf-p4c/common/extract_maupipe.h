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

#ifndef BF_P4C_COMMON_EXTRACT_MAUPIPE_H_
#define BF_P4C_COMMON_EXTRACT_MAUPIPE_H_

#include <optional>

#include "bf-p4c/arch/arch.h"
#include "bf-p4c/arch/fromv1.0/mirror.h"
#include "bf-p4c/arch/fromv1.0/resubmit.h"
#include "bf-p4c/device.h"
#include "bf-p4c/lib/assoc.h"
#include "bf-p4c/logging/source_info_logging.h"
#include "bf-p4c/mau/mau_visitor.h"
#include "bf-p4c/midend/param_binding.h"
#include "frontends/common/options.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/externInstance.h"
#include "frontends/p4/typeMap.h"
#include "ir/ir.h"

class BFN_Options;
class CreateSaluInstruction;

namespace BFN {

using namespace P4;

const IR::BFN::Pipe *extract_maupipe(const IR::P4Program *, BFN_Options &options);

/** The purpose of this code is to translate in each pipeline to Backend structures.
 *  At the top of each pipeline, each extern to be used by that pipeline is declared.
 *
 *  At the moment, we only resolve Declaration Instances to create BackendAttached tables.
 *  This may need to extend to other features at somepoint, for example Checksums, or other
 *  potential parser externs.
 *
 *  Furthermore, there is not always a one-to-one map of Declaration_Instance to BackendAttached
 *  table.  The two currently known and handled cases are:
 *      1. An action profile and a action selector share a Declaration_Instance
 *      2. A stateful selector will also share an instance with an action profile or action
 *         selector
 *
 *  These two special cases are handled individually in the code, rather than having a total
 *  map for the entire gress.  Depending on how complicated cases get, this may have to
 *  expanded at somepoint.
 */
typedef assoc::map<const IR::Declaration_Instance *, const IR::MAU::AttachedMemory *>
    DeclarationConversions;
// a mapping from Registers to the (converted) ActionSelectors that are bound to them
typedef assoc::map<const IR::Declaration_Instance *, const IR::MAU::Selector *> StatefulSelectors;

class AttachTables : public PassManager {
    P4::ReferenceMap *refMap;
    P4::TypeMap *typeMap;
    // Have to keep all non StatefulAlus separate from other Declaration_Instances, because
    // selectors and StatefulAlus may have the same Declaration
    DeclarationConversions &converted;
    assoc::map<const IR::Declaration_Instance *,
               IR::MAU::StatefulAlu *>
        salu_inits;  // Register -> StatefulAlu
    StatefulSelectors stateful_selectors;

    /**
     * \ingroup stateful_alu
     * \brief The pass converts declarations of register actions into IR::MAU::StatefulAlu nodes.
     *
     * The pass builds the BFN::AttachTables::salu_inits map, which maps declarations
     * of register parameters to IR::MAU::StatefulAlu nodes.
     *
     * There are two annotation processed in this pass:
     * 1. \@initial_register_lo_value
     * 2. \@initial_register_hi_value
     *
     * Arithmetic ALUs support the following configurations:
     * * 1/8/16/32 bits
     * * Dual mode
     */
    class InitializeStatefulAlus : public MauInspector {
        AttachTables &self;
        void postorder(const IR::Expression *) override { visitAgain(); }
        void postorder(const IR::GlobalRef *) override;
        void updateAttachedSalu(const IR::Declaration_Instance *, const IR::GlobalRef *);

     public:
        explicit InitializeStatefulAlus(AttachTables &s) : self(s) {}
    };

    /**
     * \ingroup stateful_alu
     * \brief The pass converts declarations of register parameters
     * into IR::MAU::SaluRegfileRow IR nodes.
     *
     * The pass collects all declarations of register params and checks whether
     * each of them is used in a single stateful ALU.
     * Then, it allocates a register file row for each RegisterParam used
     * in a stateful ALU and adds this information to the corresponding
     * IR::MAU::StatefulAlu::regfile map.
     *
     * Register parameters support only 8/16/32 signed and unsigned data types.
     *
     * @pre It uses information from the BFN::AttachTables::salu_inits map,
     * which is populated in the BFN::AttachTables::InitializeStatefulAlus pass.
     *
     * @post It must be called before BFN::AttachTables::InitializeStatefulInstructions,
     * which uses the information about allocated register params.
     */
    class InitializeRegisterParams : public MauInspector {
        AttachTables &self;
        ordered_map<const IR::Declaration_Instance *,
                    IR::MAU::StatefulAlu *>
            param_salus;  // RegisterParam -> StatefulAlu
        bool preorder(const IR::MAU::Primitive *prim) override;
        void end_apply() override;

     public:
        explicit InitializeRegisterParams(AttachTables &s) : self(s) {}
    };

    /**
     * \ingroup stateful_alu
     * \brief The pass converts register actions into IR::MAU::SaluAction nodes
     * using the CreateSaluInstruction inspector.
     *
     * For each SALU, it creates a single CreateSaluInstruction inspector, which is
     * applied to each IR::MAU::SaluAction node belonging to that SALU, so that information
     * can be accumulated, and creates corresponding SALU instructions.
     *
     * @pre The pass must be called after BFN::AttachTables::InitializeRegisterParams,
     * since it uses information about allocated register params.
     */
    class InitializeStatefulInstructions : public MauInspector {
        AttachTables &self;
        assoc::set<const IR::Declaration_Instance *> register_actions;
        assoc::map<const IR::MAU::StatefulAlu *, CreateSaluInstruction *> inst_ctor;

        void postorder(const IR::Expression *) override { visitAgain(); }
        void postorder(const IR::GlobalRef *gref) override;

     public:
        explicit InitializeStatefulInstructions(AttachTables &s) : self(s) {}
    };

    /**
     * \ingroup stateful_alu
     * \brief The pass attaches stateful ALUs to corresponding M/A tables.
     *
     * The attached stateful tables are stored in the IR::MAU::Table::attached vector
     * of a M/A table.
     */
    class DefineGlobalRefs : public MauModifier {
        AttachTables &self;
        P4::ReferenceMap *refMap;
        P4::TypeMap *typeMap;

        assoc::map<cstring, safe_vector<const IR::MAU::BackendAttached *>> attached;
        bool preorder(IR::MAU::Table *) override;
        bool preorder(IR::MAU::Action *) override;
        void postorder(IR::MAU::Table *) override;
        void postorder(IR::Expression *) override { visitAgain(); }
        void postorder(IR::GlobalRef *) override;

        const IR::MAU::StatefulAlu *findAttachedSalu(const IR::Declaration_Instance *ext);

     public:
        explicit DefineGlobalRefs(AttachTables &s, P4::ReferenceMap *refMap, P4::TypeMap *typeMap)
            : self(s), refMap(refMap), typeMap(typeMap) {}
    };
    bool findSaluDeclarations(const IR::Declaration_Instance *ext,
                              const IR::Declaration_Instance **reg_ptr,
                              const IR::Type_Specialized **regtype_ptr = nullptr,
                              const IR::Type_Extern **seltype_ptr = nullptr);
    profile_t init_apply(const IR::Node *root) override;

    static bool isSaluActionType(const IR::Type *);

 public:
    AttachTables(P4::ReferenceMap *rm, P4::TypeMap *tm, DeclarationConversions &con,
                 StatefulSelectors ss)
        : refMap(rm), typeMap(tm), converted(con), stateful_selectors(ss) {
        addPasses({new InitializeStatefulAlus(*this), new InitializeRegisterParams(*this),
                   new InitializeStatefulInstructions(*this),
                   new DefineGlobalRefs(*this, refMap, typeMap)});
        stop_on_error = false;
    }
};

/// must be applied to IR::BFN::Pipe
class ProcessBackendPipe : public PassManager {
 public:
    ProcessBackendPipe(P4::ReferenceMap *refMap, P4::TypeMap *typeMap, IR::BFN::Pipe *rv,
                       DeclarationConversions &converted, StatefulSelectors ss,
                       ParamBinding *bindings);
};

class BackendConverter : public Inspector {
    P4::ReferenceMap *refMap;
    P4::TypeMap *typeMap;
    ParamBinding *bindings;
    ParseTna *arch;
    const IR::ToplevelBlock *toplevel = nullptr;
    StatefulSelectors stateful_selectors;
    IR::Vector<IR::BFN::Pipe> &pipe;
    ordered_map<int, const IR::BFN::Pipe *> &pipes;
    CollectSourceInfoLogging &sourceInfoLogging;

 public:
    BackendConverter(P4::ReferenceMap *refMap, P4::TypeMap *typeMap, ParamBinding *bindings,
                     IR::Vector<IR::BFN::Pipe> &pipe,
                     ordered_map<int, const IR::BFN::Pipe *> &pipes,
                     CollectSourceInfoLogging &sourceInfoLogging)
        : refMap(refMap),
          typeMap(typeMap),
          bindings(bindings),
          pipe(pipe),
          pipes(pipes),
          sourceInfoLogging(sourceInfoLogging) {
        arch = new ParseTna(refMap, typeMap);
    }

    ordered_map<int, const IR::BFN::Pipe *> &getPipes() { return pipes; }
    cstring getPipelineName(const IR::P4Program *program, int index);
    const ProgramPipelines &getPipelines() const { return arch->pipelines; }
    bool preorder(const IR::P4Program *program) override;
};

}  // namespace BFN

#endif /* BF_P4C_COMMON_EXTRACT_MAUPIPE_H_ */
