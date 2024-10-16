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

#ifndef EXTENSIONS_BF_P4C_MIDEND_PING_PONG_GENERATION_H_
#define EXTENSIONS_BF_P4C_MIDEND_PING_PONG_GENERATION_H_

#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/typeMap.h"
#include "frontends/p4/methodInstance.h"
#include "ir/ir.h"
#include "ir/gress.h"
#include "bf-p4c/midend/type_checker.h"
#include "midend/collect_pipelines.h"

namespace BFN {

/**
 * \ingroup midend
 * \brief PassManager that adds the ping pong mechanism for ghost thread.
 * 
 * Detects registers that are used by ingress and ghost, creates
 * a duplicate tables and registers if needed and attaches
 * a ping-pong mechanism onto them.  
 *
 * There are some helpful visitors (that find/modify something specific
 * within a given sub-tree of the IR) and functions.
 * 
 * Passes work with member variables of the main PassManager (PingPongGeneration) - these
 * variables serve as a storage for saving registers/actions/tables identified for duplication.
 */
class PingPongGeneration : public PassManager {
    // CONSTANTS ----------------------------------------------------------------------------------
    /**
     * Constant suffix added to identifiers.
     */
    static const cstring ID_PING_PONG_SUFFIX;
    /**
     * Constant name of the ghost metadata structure.
     */
    static const cstring GMD_STRUCTURE_NAME;
    /**
     * Constant name of the ping_pong field.
     */
    static const cstring PING_PONG_FIELD_NAME;

    // VARIABLES ----------------------------------------------------------------------------------
    // These variables serve as a state/storage between different passes
    /**
     * Declaration of ghost_intrinsic_metadata struct
     */
    const IR::Type_Header* ghost_meta_struct = nullptr;

    // Basic maps
    P4::ReferenceMap *refMap;
    P4::TypeMap *typeMap;

    // Local maps for storing information
    using RegisterSet = std::set<const IR::Declaration_Instance *>;
    using TnaControlLess = ByNameLess<const IR::BFN::TnaControl>;

    template<typename Key>
    using TnaControlPairLess = PairLess<const IR::BFN::TnaControl *, const Key, TnaControlLess>;

    template<typename Key, typename Value>
    using TnaControlPairMap = std::map<std::pair<const IR::BFN::TnaControl *, const Key>, Value,
                                       TnaControlPairLess<const Key>>;

    template<typename Value>
    using TnaControlMap = std::map<const IR::BFN::TnaControl *, Value, TnaControlLess>;

    using RegisterToRegisterActionMap = TnaControlPairMap<const IR::Declaration_Instance *,
                                                          RegisterSet>;
    using RegisterToP4ActionMap = TnaControlPairMap<const IR::Declaration_Instance *,
                                                    std::set<const IR::P4Action *>>;
    using P4ActionToRegisterMap = TnaControlPairMap<const IR::P4Action *,
                                                    const IR::Declaration_Instance *>;
    using P4ActionToP4TableMap = TnaControlPairMap<const IR::P4Action *, const IR::P4Table *>;
    using P4TableToRegisterMap = TnaControlPairMap<const IR::P4Table *,
                                                   const IR::Declaration_Instance *>;
    using P4TableToP4TableMap = std::unordered_map<const IR::P4Table *, const IR::P4Table *>;
    using ControlToRegisterMap = TnaControlMap<RegisterSet>;
    /**
     * Array of ghost metadata names for different gresses
     */
    TnaControlMap<cstring> ghost_meta_name;

    RegisterToRegisterActionMap registerToRegisterAction;
    RegisterToP4ActionMap registerToP4Action;
    P4ActionToRegisterMap p4ActionToRegister;
    P4ActionToP4TableMap p4ActionToP4Table;
    P4TableToRegisterMap p4TableToRegister;
    ControlToRegisterMap ppIfCoveredRegisters;
    ControlToRegisterMap controlToRegister;
    P4TableToP4TableMap p4TableToDuplicateTable;

    CollectPipelines::Pipelines pipelines;

    // HELPER FUNCTIONS ---------------------------------------------------------------------------
    inline IR::Path* appendPingPongSuffix(IR::Path*, std::set<cstring>&);
    inline void duplicateNodeDeclaration(
                                const IR::Declaration*,
                                IR::BFN::TnaControl*,
                                std::set<cstring>&);

    /// Collect all registers for which we have metadata calculated, i.e. that
    /// are potential candidates for ping-pong.
    std::set<const IR::Declaration_Instance *> allRegisters() const;
    bool shouldTransform();  // FIXME const;

    /// Checks if the register valid ping-pong register for any pipeline.
    /// Usable only after all inpectors finish.
    bool isPingPongValid(const IR::Declaration_Instance *reg);

    /// Checks if the register valid ping-pong register for any pipeline that
    /// uses given gress.
    /// Usable only after all inpectors finish.
    bool isPingPongValid(const IR::BFN::TnaControl *gress,
                         const IR::Declaration_Instance *reg,
                         bool calculatingCovered = false);  // FIXME const;

    /// Checks if the register valid ping-pong register for the given pipeline.
    /// Usable only after all inpectors finish.
    bool isPingPongValid(const CollectPipelines::Pipe &pipe,
                         const IR::Declaration_Instance *reg,
                         bool calculatingCovered = false);  // FIXME const;

    // HELPER VISITORS ----------------------------------------------------------------------------
    // Finds a ghost_metadata.ping_pong field reference in a subtree
    class PingPongFieldFinder;
    // Base class for chaning declarations
    class DeclarationChanger;
    // This visitor changes specific references in new cloned register action
    class RegActionChanger;
    // This visitor changes specific references in new cloned P4 action
    class P4ActionChanger;
    // This visitor changes specific references in new cloned P4 table
    class P4TableChanger;
    // This visitor changes P4 table references in cloned MethodCallStatement
    class ApplyMCSChanger;

    // MAIN VISITORS ------------------------------------------------------------------------------
    /**
     * \brief Gets all of the registers used and their actions.
     */
    class GetAllRegisters: public Inspector {
        PingPongGeneration &self;

        bool preorder(const IR::MethodCallExpression*) override;
     public:
        explicit GetAllRegisters(PingPongGeneration &self) :
             self(self) {}
    };

    /**
     * \brief Finds and adds all of the corresponding tables.
     */
    class AddAllTables: public Inspector {
        PingPongGeneration &self;

        bool preorder(const IR::ActionListElement*) override;
     public:
        explicit AddAllTables(PingPongGeneration &self) :
             self(self) {}
    };

    /**
     * \brief Checks for tables that are already applied under ping_pong condition.
     * 
     * Also checks if ghost_metadata are even present or if there are multiple pipelines
     */
    class CheckPingPongTables: public Inspector {
        PingPongGeneration &self;
        unsigned pipes = 0;

        // Finds ghost_metadata structure presence
        bool preorder(const IR::Type_Header*) override;
        // Finds tables applied under ping_pong
        bool preorder(const IR::PathExpression*) override;
        // Gets the identifier of ghost metadata
        bool preorder(const IR::Parameter*) override;
     public:
        explicit CheckPingPongTables(PingPongGeneration &self) :
             self(self) {}
    };

    /**
     * \brief Duplicates all of the required tables, registers, actions.
     */
    class GeneratePingPongMechanismDeclarations: public Transform {
        PingPongGeneration &self;

        IR::Node *preorder(IR::P4Program*);
        IR::Node *preorder(IR::BFN::TnaControl*);
     public:
        explicit GeneratePingPongMechanismDeclarations(PingPongGeneration &self) :
             self(self) {}
    };

    /**
     * \brief Adds PingPong mechanism/if statement.
     */
    class GeneratePingPongMechanism: public Transform {
        PingPongGeneration &self;

        IR::Node *postorder(IR::MethodCallStatement*);
     public:
        explicit GeneratePingPongMechanism(PingPongGeneration &self) :
             self(self) {}
    };

 public:
    /**
     * Constructor that adds all of the passes.
     */
    PingPongGeneration(P4::ReferenceMap* refMap, P4::TypeMap* typeMap) :
         refMap(refMap), typeMap(typeMap) {
        setName("Automatic ping-pong generation");
        addPasses({
            new CollectPipelines(refMap, &pipelines),
            new TypeChecking(refMap, typeMap, true),
            new GetAllRegisters(*this),
            new AddAllTables(*this),
            new CheckPingPongTables(*this),
            new PassIf([this]() { return shouldTransform(); }, {
                new GeneratePingPongMechanismDeclarations(*this),
                // Update ref and type map, bacause the IR might have changed
                new TypeChecking(refMap, typeMap, true),
                new GeneratePingPongMechanism(*this)
            })
        });
    }
};

}  // namespace BFN

#endif  // EXTENSIONS_BF_P4C_MIDEND_PING_PONG_GENERATION_H_
