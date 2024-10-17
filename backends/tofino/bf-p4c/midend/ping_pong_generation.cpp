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
 *  Detects registers that are used by ingress and ghost, creates
 *  a duplicate tables and registers if needed and attaches
 *  a ping-pong mechanism onto them.  
 *
 *  This file contains implementation of different methods, refer to the header file
 *  for more abstract overview to determine what each method should do and is used for.
 */
#include "ping_pong_generation.h"
#include "lib/set.h"

namespace BFN {

// CONSTANTS --------------------------------------------------------------------------------------
const cstring PingPongGeneration::ID_PING_PONG_SUFFIX = "__gen_duplicate_ping_pong"_cs;
const cstring PingPongGeneration::GMD_STRUCTURE_NAME = "ghost_intrinsic_metadata_t"_cs;
const cstring PingPongGeneration::PING_PONG_FIELD_NAME = "ping_pong"_cs;

// HELPER VISITORS --------------------------------------------------------------------------------
/**
 * \brief Base visitor class for other classes that change declarations.
 */
class PingPongGeneration::DeclarationChanger: public Transform {
 public:
    PingPongGeneration &self;
    std::set<cstring> names_to_change;

    /**
     * Update StringLiteral
     */
    IR::Node *postorder(IR::StringLiteral* sl) {
        // Check if this is within "name" Annotation
        auto annot = findContext<IR::Annotation>();
        if (!annot || annot->name != "name")
            return sl;
        // Check if this is one of the identifiers that are being worked on
        // (use only the name after the last ".")
        std::string local_name(sl->value);
        size_t pos = 0;
        while ((pos = local_name.find(".")) != std::string::npos)
            local_name.erase(0, pos + 1);
        if (!names_to_change.count(local_name))
            return sl;
        // Update and return new literal
        sl->value = sl->value + PingPongGeneration::ID_PING_PONG_SUFFIX;
        return sl;
    }

    explicit DeclarationChanger(PingPongGeneration &self,
                                std::set<cstring> &names_to_change) :
         self(self), names_to_change(names_to_change) {}
};


/**
 * \brief This visitor changes specific references in new cloned register action.
 */
class PingPongGeneration::RegActionChanger: public PingPongGeneration::DeclarationChanger {
    // Variables and updating of proper StringLiteral are inherited from DeclarationChanger
    /**
     * Update path to register
     */
    IR::Node *postorder(IR::Path* path) {
        // Check if we are in an argument
        auto arg = findContext<IR::Argument>();
        if (!arg)
            return path;
        // Clone and update new path
        return self.appendPingPongSuffix(path, names_to_change);
    }

 public:
    explicit RegActionChanger(PingPongGeneration &self,
                              std::set<cstring> &names_to_change) :
         DeclarationChanger(self, names_to_change) {}
};

/**
 * \brief This visitor changes specific references in new cloned P4 action.
 */
class PingPongGeneration::P4ActionChanger: public PingPongGeneration::DeclarationChanger {
    // Variables and updating of proper StringLiteral are inherited from DeclarationChanger
    /** 
     * Update path to register action
     */
    IR::Node *postorder(IR::Path* path) {
        // Check for "execute"/"count"
        // as we want to try and change all ocurences of register/counter/meter invocations
        auto member = findContext<IR::Member>();
        if (!member || (member->member != "execute" && member->member != "count"))
            return path;
        // Clone and update new path
        return self.appendPingPongSuffix(path, names_to_change);
    }

 public:
    explicit P4ActionChanger(PingPongGeneration &self,
                             std::set<cstring> &names_to_change) :
        DeclarationChanger(self, names_to_change) {}
};

/**
 * \brief This visitor changes specific references in new cloned P4 table.
 */
class PingPongGeneration::P4TableChanger: public PingPongGeneration::DeclarationChanger {
    // Variables and updating of proper StringLiteral are inherited from DeclarationChanger
    /**
     * Update path to p4 action
     */
    IR::Node *postorder(IR::Path* path) {
        // Check for "default_action"/"counters"/"meters"/"registers" property
        // as we want to try and change one of those properties
        auto prop = findContext<IR::Property>();
        // Check for ActionListElement
        // as we also want to change regular actions
        auto ale = findContext<IR::ActionListElement>();
        // If neither is correct, stop
        if (!prop ||
            (prop->name != "default_action" && prop->name != "counters" &&
             prop->name != "registers" && prop->name != "meters"))
            if (!ale)
                return path;
        // Clone and update new path
        return self.appendPingPongSuffix(path, names_to_change);
    }

 public:
    explicit P4TableChanger(PingPongGeneration &self,
                            std::set<cstring> &names_to_change) :
         DeclarationChanger(self, names_to_change) {}
};

/**
 * \brief This visitor changes P4 table references in cloned MethodCallStatement.
 */
class PingPongGeneration::ApplyMCSChanger: public Transform {
    PingPongGeneration &self;
    const cstring orig_table_name;
    const IR::P4Table* dupl_table;
    std::set<cstring> orig_action_names;

    /**
     * Explicit visits to "type" member variables
     */
    IR::Node *preorder(IR::Expression* exp) {
        visit(exp->type, "type");
        return exp;
    }
    IR::Node *preorder(IR::Member* member) {
        visit(member->type, "type");
        return member;
    }
    /**
     * Swap p4 table for duplicate table
     */
    IR::Node *preorder(IR::P4Table* tab) {
        // Check if this is the worked on table
        if (tab->name != orig_table_name)
            return tab;
        // Return the duplicate table
        return dupl_table->clone();
    }
    /**
     * Update name of p4 table
     */
    IR::Node *postorder(IR::Type_Struct* ts) {
        // Check if this is one of the identifiers that are being worked on
        if (ts->name != orig_table_name)
            return ts;
        // Create a duplicate and update it
        return new IR::Type_Struct(IR::ID(ts->name + ID_PING_PONG_SUFFIX),
                                   ts->annotations, ts->fields);
    }
    /**
     * Update path to p4 table
     */
    IR::Node *postorder(IR::Path* path) {
        std::set<cstring> names_to_change;
        names_to_change.insert(orig_action_names.begin(),
                               orig_action_names.end());
        names_to_change.insert(orig_table_name);
        // Clone and update new path
        return self.appendPingPongSuffix(path, names_to_change);
    }

 public:
    explicit ApplyMCSChanger(PingPongGeneration &self,
                             const cstring& orig_table_name,
                             const IR::P4Table* dupl_table,
                             std::set<cstring>& orig_action_names) :
         self(self), orig_table_name(orig_table_name),
         dupl_table(dupl_table), orig_action_names(orig_action_names) {}
};

/**
 * \brief Finds a ghost_metadata.ping_pong field reference in a subtree.
 */
class PingPongGeneration::PingPongFieldFinder: public Inspector {
    bool preorder(const IR::Member* member) override {
        // We only care about ping_pong
        if (member->member != PING_PONG_FIELD_NAME)
            return false;
        // Get PathExpression
        auto path_exp = member->expr->to<IR::PathExpression>();
        if (!path_exp)
            return false;
        // Check if this is part of metadata
        if (path_exp->type->is<IR::Type_Header>()
            || path_exp->type->is<IR::Type_Struct>()) {
            found = true;
        }
        return false;
    }

 public:
    bool found;
    PingPongFieldFinder() :
         found(false) {}
};

// HELPER FUNCTIONS -------------------------------------------------------------------------------
/**
 * \brief Creates and updates duplicate of a path.
 * 
 * @param[in] path Path that should be duplicated and updated.
 * @param[in] names_to_change Set of names that were identified to be changed.
 * @return Returns the original path (in case it is not supposed to be updated)
 *         or a duplicate of the path that was updated to include the ID_PING_PONG_SUFFIX.
 */
inline IR::Path* PingPongGeneration::appendPingPongSuffix(IR::Path* path,
                                                          std::set<cstring>& names_to_change) {
    // Check if this is one of the identifiers that are being worked on
    if (!names_to_change.count(path->name))
        return path;
    // Create a duplicate and update it
    path->name = IR::ID(path->name + ID_PING_PONG_SUFFIX);
    return path;
}

/**
 * \brief Creates and updates a duplicate of given declaration and places
 *        it into the gress object.
 * 
 * @param[in] node Node that should be duplicated and updated.
 * @param[inout] gress Gress of the node (that will be changed within it).
 * @param[in] names_to_change Set of names that were identified to be changed.
 * 
 * @sa RegActionChanger
 * @sa P4ActionChanger
 * @sa P4TableChanger
 */
inline void PingPongGeneration::duplicateNodeDeclaration(const IR::Declaration* node,
                                                         IR::BFN::TnaControl* gress,
                                                         std::set<cstring>& names_to_change) {
    // Create new duplicate node and apply changes via visitors
    const IR::Node* changed_new_node = node;
    auto new_name = IR::ID(node->name + ID_PING_PONG_SUFFIX);
    if (node->is<IR::Declaration_Instance>()) {
        auto DI_node = node->to<IR::Declaration_Instance>();
        auto new_node = new IR::Declaration_Instance(
                                new_name,
                                DI_node->annotations,
                                DI_node->type,
                                DI_node->arguments,
                                DI_node->initializer);
        changed_new_node = new_node->apply(RegActionChanger(*this, names_to_change));
    } else if (node->is<IR::P4Action>()) {
        auto P4A_node = node->to<IR::P4Action>();
        auto new_node = new IR::P4Action(
                                new_name,
                                P4A_node->annotations,
                                P4A_node->parameters,
                                P4A_node->body);
        changed_new_node = new_node->apply(P4ActionChanger(*this, names_to_change));
    } else if (node->is<IR::P4Table>()) {
        auto P4T_node = node->to<IR::P4Table>();
        auto new_node = new IR::P4Table(
                                new_name,
                                P4T_node->annotations,
                                P4T_node->properties);
        changed_new_node = new_node->apply(P4TableChanger(*this, names_to_change));
        // Also save the duplicate table for later use
        p4TableToDuplicateTable[node->to<IR::P4Table>()] = changed_new_node->to<IR::P4Table>();
    } else {
        return;
    }
    LOG3("[PPG::GPPMD] Created duplicate declaration: " << new_name);
    // Add it into gress declarations (right next to the original one)
    gress->controlLocals.insert(
        std::find(gress->controlLocals.begin(), gress->controlLocals.end(), node),
        changed_new_node->to<IR::Declaration>());
    return;
}

std::set<const IR::Declaration_Instance *> PingPongGeneration::allRegisters() const {
    std::set<const IR::Declaration_Instance *> all;
    for (auto &kv : controlToRegister)
        for (auto reg : kv.second)
            all.insert(reg);

    return all;
}

bool PingPongGeneration::shouldTransform() {
    std::map<const CollectPipelines::Pipe *, RegisterSet> validInPipe;
    bool anyValid = false;
    auto registers = allRegisters();
    for (auto &pipe : pipelines) {
        if (!pipe.ghost)
            continue;

        for (auto reg : registers) {
            if (isPingPongValid(pipe, reg)) {
                LOG3("[PPG] Register: " << reg->name << " seems to be valid for PPG on pipeline "
                     << pipe.dec->getName() << ".");
                validInPipe[&pipe].insert(reg);
                anyValid = true;
            }
        }
    }
    if (!anyValid) {
        LOG6("[PPG] No transformation is needed");
        return false;
    }

    LOG6("[PPG] There are registers to transform");
    for (auto &p1 : pipelines) {
        if (!p1.ghost || validInPipe[&p1].empty())
            continue;

        for (auto &p2 : pipelines) {
            // looking for different pipes with host and intersecting set of registers used in
            // ping-pong
            if (&p1 == &p2 || !p2.ghost || !intersects(validInPipe[&p1], validInPipe[&p2]))
                continue;
            // We can safely transform if either ingress & ghost are the same in both, or they
            // are both different. If we have same control and different ghosts or different
            // controls but same ghost we would need to duplicate the shared resource (TODO).
            if ((p1.ingress.control == p2.ingress.control) != (p1.ghost == p2.ghost)) {
                warning(ErrorType::WARN_UNSUPPORTED,
                        "Fould registers for ping-pong mechanism generation, but the "
                        "ingress-ghost pairs are overlapping between pipes. This "
                        "transformation is not yet supported. Conflict between pipes %1% "
                        "and %2%.",
                        p1.dec, p2.dec);
                return false;
            }
        }
    }
    return true;
}

bool PingPongGeneration::isPingPongValid(const IR::Declaration_Instance *reg) {
    for (auto &pipe : pipelines)
        if (isPingPongValid(pipe, reg))
            return true;
    return false;
}

bool PingPongGeneration::isPingPongValid(const IR::BFN::TnaControl *gress,
                                         const IR::Declaration_Instance *reg,
                                         bool calculatingCovered) {
    for (auto &pipe : pipelines.get(gress))
        if (isPingPongValid(pipe, reg, calculatingCovered))
            return true;
    return false;
}

bool PingPongGeneration::isPingPongValid(const CollectPipelines::Pipe &pipe,
                                         const IR::Declaration_Instance *reg,
                                         bool calculatingCovered) {
    //                 register used in egrees => cannot be pingpong
    if (!pipe.ghost || controlToRegister[pipe.egress.control].count(reg))
        return false;

    auto itIn = registerToP4Action.find({pipe.ingress.control, reg});
    auto itGh = registerToP4Action.find({pipe.ghost, reg});
    if (itIn != registerToP4Action.end() && !itIn->second.empty()
        && itGh != registerToP4Action.end() && !itIn->second.empty())
    {
        // if the register is already covered by if ping_pong we do not transform it
        bool coveredIn = ppIfCoveredRegisters[pipe.ingress.control].count(reg);
        bool coveredGhost = ppIfCoveredRegisters[pipe.ghost].count(reg);
        if (!calculatingCovered && (coveredIn || coveredGhost)) {
            if (!(coveredIn && coveredGhost)) {
                warning(ErrorType::WARN_MISMATCH, "Register %1% is covered by ping-pong 'if' in "
                          "%2% but not in corresponding %3%, not going to auto-generate "
                          "ping-poing 'if' for inconsistently used register.",
                          reg, coveredIn ? pipe.ingress.control->getName() : pipe.ghost->getName(),
                          coveredIn ? pipe.ghost->getName() : pipe.ingress.control->getName());
            }
            return false;
        }

        return true;
    }
    return false;
}

// MAIN VISITORS ----------------------------------------------------------------------------------
/**
 * Gets all of the registers used and their actions.
 */
bool PingPongGeneration::GetAllRegisters::preorder(const IR::MethodCallExpression* mce) {
    // Find the instance method of RegisterAction
    auto mi = P4::MethodInstance::resolve(mce, self.refMap, self.typeMap);
    if (!mi)
        return false;
    auto em = mi->to<P4::ExternMethod>();
    if (!em || !em->originalExternType->name.name.startsWith("RegisterAction"))
        return false;
    auto ra_decl = mi->object->to<IR::Declaration_Instance>();
    if (!ra_decl || ra_decl->arguments->size() < 1 ||
        !ra_decl->arguments->at(0)->expression->is<IR::PathExpression>())
        return false;
    // Get the register out of the register action
    auto r_path = ra_decl->arguments->at(0)->expression->to<IR::PathExpression>()->path;
    if (!r_path)
        return false;
    auto r_decl = self.refMap->getDeclaration(r_path)->to<IR::Declaration_Instance>();
    if (!r_decl)
        return false;
    // Get gress and P4Action
    auto gress = findContext<IR::BFN::TnaControl>();
    if (!gress)
        return false;
    auto p4_action = findContext<IR::P4Action>();
    if (!p4_action)
        return false;

    self.controlToRegister[gress].insert(r_decl);
    // Check if this action doesn't work with multiple registers
    // the register can be nullptr if it was already invalidated
    auto other_reg_it = self.p4ActionToRegister.find({gress, p4_action});
    if (other_reg_it != self.p4ActionToRegister.end()) {
        // Invalidate this action and everything attached to it
        LOG4("[PPG::GAR] Invalidating P4 action " << p4_action->name <<
             " that uses multiple registers.");
        if (auto other_reg = other_reg_it->second) {
            self.registerToRegisterAction.erase({gress, other_reg});
            self.registerToP4Action.erase({gress, other_reg});
        }
        self.registerToRegisterAction.erase({gress, r_decl});
        self.registerToP4Action.erase({gress, r_decl});
        // Invalidate permanentry, don't erase (it could be problem if 3 registers use the action)
        self.p4ActionToRegister[{gress, p4_action}] = nullptr;
    } else {
        // Add the register action
        LOG4("[PPG::GAR] adding register with declaration: " << r_decl->name <<
            "; register action declaration: " << ra_decl->name <<
            "; p4 action: " << p4_action->name);
        self.registerToRegisterAction[{gress, r_decl}].insert(ra_decl);
        self.registerToP4Action[{gress, r_decl}].insert(p4_action);
        self.p4ActionToRegister[{gress, p4_action}] = r_decl;
    }
    return false;
}

/**
 * Finds and adds all of the corresponding tables.
 * Works on call expression of the P4 action within a P4 table's action list.
 */
bool PingPongGeneration::AddAllTables::preorder(const IR::ActionListElement* ale) {
    // If we are not within a Table we can ignore this
    auto p4_table = findContext<IR::P4Table>();
    if (!p4_table)
        return false;

    // Check if this has MethodCallExpression
    auto mce = ale->expression->to<IR::MethodCallExpression>();
    if (!mce)
        return false;

    // Resolve the method
    auto mi = P4::MethodInstance::resolve(mce, self.refMap, self.typeMap);
    if (!mi)
        return false;
    auto ac = mi->to<P4::ActionCall>();
    if (!ac)
        return false;
    auto p4_action = ac->action;
    if (!p4_action)
        return false;

    // Find gress
    auto gress = findContext<IR::BFN::TnaControl>();
    if (!gress)
        return false;

    // Check if it works with register
    auto regIt = self.p4ActionToRegister.find({gress, p4_action});
    if (regIt == self.p4ActionToRegister.end() || !regIt->second)
        return false;

    // Get the register
    auto reg = regIt->second;

    // Check if we already have a table for the action/register or
    // register for the table
    if (self.p4ActionToP4Table.count({gress, p4_action}) ||
        (self.p4TableToRegister.count({gress, p4_table}) &&
         self.p4TableToRegister[{gress, p4_table}] != reg)) {
        // There should only be one to one mapping, this is not ping-pong
        // Invalidate any other register that might not satisfy this
        const IR::Declaration_Instance *other_reg = nullptr;
        if (self.p4TableToRegister.count({gress, p4_table}))
            other_reg = self.p4TableToRegister[{gress, p4_table}];
        if (other_reg && other_reg != reg) {
            LOG3("[PPG::AAT] Register " << other_reg->name <<
                " does not have 1-1 mapping to P4 tables => not a ping-pong");
            self.registerToRegisterAction.erase({gress, other_reg});
            self.registerToP4Action.erase({gress, other_reg});
        }
        // Invalidate the register
        LOG3("[PPG::AAT] Register " << reg->name <<
             " does not have 1-1 mapping to P4 tables => not a ping-pong");
        self.registerToRegisterAction.erase({gress, reg});
        self.registerToP4Action.erase({gress, reg});
        self.p4ActionToRegister[{gress, p4_action}] = nullptr;
        self.p4ActionToP4Table.erase({gress, p4_action});
        self.p4TableToRegister.erase({gress, p4_table});
    } else {
        LOG4("[PPG::AAT] found table for register " << reg->name <<
             "; p4 action: " << p4_action->name <<
             "; p4 table: " << p4_table->name);
        self.p4ActionToP4Table[{gress, p4_action}] = p4_table;
        self.p4TableToRegister[{gress, p4_table}] = reg;
    }

    return false;
}

/**
 * Gets the identifier of ghost metadata.
 * This visits the parameter and checks if it is an identifier for ghost metadata.
 */
bool PingPongGeneration::CheckPingPongTables::preorder(const IR::Parameter* param) {
    // We are interested in ghost metadata paths only
    if (!param->type->is<IR::Type_Name>() ||
        param->type->to<IR::Type_Name>()->path->name != GMD_STRUCTURE_NAME)
        return false;
    // We also have to be in a gress
    auto gress = findContext<IR::BFN::TnaControl>();
    if (!gress)
        return false;
    // Parameter name is the name of ghost metadata
    self.ghost_meta_name[gress] = param->name;
    LOG5("[PPG::CPPT] Found ghost md name: " << param->name << " for gress: " << gress->thread);
    return false;
}
// Find if ghost metadata is used
bool PingPongGeneration::CheckPingPongTables::preorder(const IR::Type_Header* th) {
    // We are interested in ghost metadata only
    if (th->name != GMD_STRUCTURE_NAME)
        return false;
    // Store them
    self.ghost_meta_struct = th;
    LOG5("[PPG::CPPT] Found ghost metadata structure: " << th);
    return false;
}

/**
 * Checks for tables that are already applied under ping_pong condition.
 * We are working on table.apply() and looking at the context it is called
 * from - more specifically if it is called from within IF statement
 * that looks something like "if (ghost_meta.ping_pong == ...)".
 */
bool PingPongGeneration::CheckPingPongTables::preorder(const IR::PathExpression* path_exp) {
    // We are looking for Type_Table
    if (!path_exp->type->is<IR::Type_Table>())
        return false;

    // Check if we are inside apply
    auto member = findContext<IR::Member>();
    if (!member || member->member != "apply")
        return false;

    // Get Table declaration
    auto p4_table = self.refMap->getDeclaration(path_exp->path)->to<IR::P4Table>();
    if (!p4_table)
        return false;

    // Get gress info
    auto gress = findContext<IR::BFN::TnaControl>();
    if (!gress)
        return false;

    // Get the approriate register
    if (!self.p4TableToRegister.count({gress, p4_table}))
        return false;
    auto reg = self.p4TableToRegister[{gress, p4_table}];

    // Check if this is still valid ping-pong candidate
    if (!self.isPingPongValid(gress, reg, true))
        return false;

    // Check if there is already ping-pong mechanism attached
    // If it is we can invalidate this candidate
    // Get the IfStatement
    auto if_stmt = findContext<IR::IfStatement>();
    if (!if_stmt || !if_stmt->condition )
        return false;

    // Find the ping_pong field
    PingPongFieldFinder findPingPongField;
    if_stmt->condition->apply(findPingPongField);
    // If it was found invalidate this for ping pong generation
    if (findPingPongField.found) {
        LOG3("[PPG::CPPT] Found ping pong mechanism for: " << reg->name
             << " in gress " << gress->getName() << " => invalidating.");
        self.ppIfCoveredRegisters[gress].insert(reg);
    }

    return false;
}

/**
 * Duplicates all of the required registers.
 */
IR::Node *PingPongGeneration::GeneratePingPongMechanismDeclarations::preorder(IR::P4Program* prog) {
    // For every register
    for (auto const &reg : self.allRegisters()) {
        // That was found to be valid for ping pong duplication in any pipe
        if (self.isPingPongValid(reg)) {
            // Create duplicate register
            auto new_reg = new IR::Declaration_Instance(
                                IR::ID(reg->name + PingPongGeneration::ID_PING_PONG_SUFFIX),
                                reg->annotations,
                                reg->type,
                                reg->arguments,
                                reg->initializer);
            LOG3("[PPG::GPPMD] Created duplicate register: " << new_reg->name);
            // Place it into the P4Program's objects
            prog->objects.insert(std::find(prog->objects.begin(), prog->objects.end(), reg),
                                 new_reg);
        }
    }

    return prog;
}
/**
 * Duplicates all of the required tables, actions.
 */
IR::Node *PingPongGeneration::GeneratePingPongMechanismDeclarations::preorder(
                                                        IR::BFN::TnaControl* gress) {
    // We dont care about egress
    if (gress->thread == EGRESS)
        return gress;

    for (auto const &reg : self.allRegisters()) {
        // is it valid in any pipe that uses this gress? (FIXME)
        if (self.isPingPongValid(gress, reg)) {
            if (!self.registerToRegisterAction.count({gress, reg}) ||
                !self.registerToP4Action.count({gress, reg}))
                continue;
            auto reg_actions = self.registerToRegisterAction[{gress, reg}];
            auto p4_actions = self.registerToP4Action[{gress, reg}];
            if (p4_actions.empty())
                continue;
            auto p4_action = *p4_actions.begin();
            auto p4_table = self.p4ActionToP4Table[{gress, p4_action}];
            if (!p4_table)
                continue;
            // We need to duplicate any attached direct resources as well as they cannot
            // be attached to two tables (the original one and the duplicated table)
            std::vector<const IR::Declaration_Instance*> direct_res;
            if (auto props = p4_table->properties) {
                for (auto prop : props->properties) {
                    if (prop->name == "counters" || prop->name == "registers" ||
                        prop->name == "meters") {
                        auto val = prop->value->to<IR::ExpressionValue>();
                        if (!val) continue;
                        auto pe = val->expression->to<IR::PathExpression>();
                        if (!pe) continue;
                        auto res_dec = self.refMap->getDeclaration(pe->path);
                        if (!res_dec) continue;
                        auto res_decl = res_dec->to<IR::Declaration_Instance>();
                        if (!res_decl) continue;
                        direct_res.push_back(res_decl);
                        LOG4("[PPG::GPPMD] found direct resource " << res_decl->name <<
                            " for p4 table: " << p4_table->name);
                    }
                }
            }
            // Create a vector of names that are worked on
            std::set<cstring> names_to_change = { reg->name,
                                                  p4_table->name, reg->name.originalName,
                                                  p4_table->name.originalName };
            for (auto res : direct_res) {
                names_to_change.insert(res->name);
                names_to_change.insert(res->name.originalName);
            }
            for (auto reg_action : reg_actions) {
                names_to_change.insert(reg_action->name);
                names_to_change.insert(reg_action->name.originalName);
            }
            for (auto p4_action : p4_actions) {
                names_to_change.insert(p4_action->name);
                names_to_change.insert(p4_action->name.originalName);
            }
            for (auto res : direct_res) {
                // Create duplicate direct resoures
                self.duplicateNodeDeclaration(res, gress, names_to_change);
            }
            for (auto reg_action : reg_actions)
                // Create duplicate register action
                self.duplicateNodeDeclaration(reg_action, gress, names_to_change);
            for (auto p4_action : p4_actions)
                // Create duplicate P4 Action
                self.duplicateNodeDeclaration(p4_action, gress, names_to_change);
            // Create duplicate P4 Table
            self.duplicateNodeDeclaration(p4_table, gress, names_to_change);
        }
    }

    return gress;
}

/**
 * Adds PingPong mechanism/if statement.
 */
IR::Node *PingPongGeneration::GeneratePingPongMechanism::postorder(IR::MethodCallStatement* mcs) {
    // Check gress
    auto gress = findContext<IR::BFN::TnaControl>();
    if (!gress)
        return mcs;
    // We dont care about egress
    if (gress->thread == EGRESS)
        return mcs;

    // Check if this has apply call of a table
    auto mi = P4::MethodInstance::resolve(mcs, self.refMap, self.typeMap);
    if (!mi)
        return mcs;
    auto am = mi->to<P4::ApplyMethod>();
    if (!am || !am->isTableApply())
        return mcs;
    auto p4_table = mi->object->to<IR::P4Table>();
    if (!p4_table)
        return mcs;
    // Get the approriate register
    if (!self.p4TableToRegister.count({gress, p4_table}))
        return mcs;
    auto reg = self.p4TableToRegister[{gress, p4_table}];
    // Check if this is valid ping-pong candidate
    if (!self.isPingPongValid(gress, reg))
        return mcs;
    // Get the duplicate of this table
    if (!self.p4TableToDuplicateTable.count(p4_table))
        return mcs;
    auto duplicate_table = self.p4TableToDuplicateTable[p4_table];
    if (!duplicate_table)
        return mcs;
    // Get the action for this table
    if (!self.registerToP4Action.count({gress, reg}))
        return mcs;
    std::set<cstring> p4_actions_names;
    auto p4_actions = self.registerToP4Action[{gress, reg}];
    for (auto p4_action : p4_actions)
        p4_actions_names.insert(p4_action->name);

    // Update the mcs
    auto new_mcs = mcs->apply(
                        ApplyMCSChanger(self, p4_table->name, duplicate_table, p4_actions_names))
                   ->to<IR::MethodCallStatement>();

    // Check existence of ghost metadata
    if (!self.ghost_meta_struct)
        return mcs;
    for (auto pipe : self.pipelines.get(gress)) {
        if (self.ghost_meta_name[pipe.ingress.control] == "" ||
            self.ghost_meta_name[pipe.ghost] == "")
            return mcs;
    }
    // Left expression (ghost_metadata.ping_pong)
    auto left = new IR::Member(IR::Type::Bits::get(1),
                               new IR::PathExpression(self.ghost_meta_struct,
                                                      new IR::Path(
                                                        IR::ID(self.ghost_meta_name[gress]),
                                                      false)),
                               IR::ID(PING_PONG_FIELD_NAME));
    // Right expression - 0 or 1 based on if this is ingress or ghost
    auto right = new IR::Constant(IR::Type::Bits::get(1), (gress->thread == INGRESS) ? 0 : 1);
    LOG3("[PPG::GPPM] Created ping-pong if statement.");
    // Return new if statement
    return new IR::IfStatement(new IR::Equ(IR::Type_Boolean::get(), left, right), mcs, new_mcs);
}

}   // namespace BFN
