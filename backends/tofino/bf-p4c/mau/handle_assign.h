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

#ifndef EXTENSIONS_BF_P4C_MAU_HANDLE_ASSIGN_H_
#define EXTENSIONS_BF_P4C_MAU_HANDLE_ASSIGN_H_

#include "bf-p4c/mau/mau_visitor.h"
#include "bf-p4c/phv/phv_fields.h"
#include "bf-p4c/mau/ixbar_expr.h"

using namespace P4;

/**
 * The purpose of this pass is to assign the action handle for the context JSON node
 * in the compiler. This is required because the actions are no longer associated with the
 * action data table in the assembler, but with the match table instead.
 *
 * This will also validate some constraints currently imposed by Brig's allocation of
 * actions on action profiles, mainly that the actions for each action profile must be the
 * same action, (at least the same action name and parameters)
 */
class AssignActionHandle : public PassManager {
    class ActionProfileImposedConstraints : public MauInspector {
        ordered_map<const IR::MAU::ActionData *, std::set<cstring>> profile_actions;
        bool preorder(const IR::MAU::ActionData *) override;
        bool preorder(const IR::MAU::Table *) override { visitOnce(); return true; }

     public:
        ActionProfileImposedConstraints() { visitDagOnce = false; }
    };

    typedef ordered_map<const IR::MAU::Action *, unsigned> HandleAssignments;
    HandleAssignments handle_assignments;

    class DetermineHandle : public MauInspector {
        // 0 - phase0 table action handle
        static constexpr unsigned INIT_ACTION_HANDLE = (0x20 << 24) + 1;
        unsigned handle_position = 0;

        AssignActionHandle &self;
        typedef ordered_map<cstring, unsigned> ProfileAssignment;
        ordered_map<const IR::MAU::ActionData *, ProfileAssignment> profile_assignments;
        bool preorder(const IR::MAU::Action *) override;
        profile_t init_apply(const IR::Node *root) override;
        unsigned next_handle() {
            unsigned rv = INIT_ACTION_HANDLE + handle_position;
            handle_position++;
            return rv;
        }

     public:
        explicit DetermineHandle(AssignActionHandle &aah) : self(aah) { }
    };

    class AssignHandle : public MauModifier {
        const AssignActionHandle &self;
        bool preorder(IR::MAU::Action *) override;

     public:
        explicit AssignHandle(const AssignActionHandle &aah) : self(aah) { }
    };

    class GuaranteeUniqueHandle : public MauInspector {
        ordered_set<const IR::MAU::ActionData *> action_profiles;
        std::map<unsigned, const IR::MAU::Action *> unique_handle;
        profile_t init_apply(const IR::Node *root) override;
        bool preorder(const IR::MAU::Action *) override;

     public:
        GuaranteeUniqueHandle() {}
    };

    class ValidateSelectors : public PassManager {
        ordered_map<const IR::MAU::Selector *, P4HashFunction *> selector_keys;
        ordered_map<const IR::MAU::Selector *, const IR::MAU::Table *> initial_table;
        ordered_map<const IR::MAU::Table *, const IR::MAU::Selector *> table_to_selector;
        const PhvInfo &phv;

        profile_t init_apply(const IR::Node *root) override;



        class ValidateKey : public MauInspector {
            ValidateSelectors &self;

            bool preorder(const IR::MAU::Selector *sel) override;
            bool preorder(const IR::MAU::Table *) override { visitOnce(); return true; }

         public:
            explicit ValidateKey(ValidateSelectors &s) : self(s) { visitDagOnce = false; }
        };

        class SetSymmetricSelectorKeys : public MauModifier {
            ValidateSelectors &self;
            bool preorder(IR::MAU::Table *tbl) override;

         public:
            explicit SetSymmetricSelectorKeys(ValidateSelectors &s) : self(s) { }
        };



     public:
        explicit ValidateSelectors(const PhvInfo &phv) : phv(phv) {
            addPasses({
                new ValidateKey(*this),
                new SetSymmetricSelectorKeys(*this)
            });
        }
    };



 public:
    explicit AssignActionHandle(const PhvInfo &phv) {
        addPasses({
            new ValidateSelectors(phv),
            new ActionProfileImposedConstraints,
            new DetermineHandle(*this),
            new AssignHandle(*this),
            new GuaranteeUniqueHandle
        });
    }
};

#endif  /* EXTENSIONS_BF_P4C_MAU_HANDLE_ASSIGN_H_ */
