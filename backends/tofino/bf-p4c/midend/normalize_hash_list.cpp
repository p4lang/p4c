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

#include "normalize_hash_list.h"

#include <iterator>
#include <map>
#include <vector>

#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/moveDeclarations.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/p4/typeMap.h"

namespace {

class TemporaryVariableInserter {
 private:
    const ::P4::ReferenceMap *ref_map;

    typedef std::vector<IR::Declaration *> Declarations;
    Declarations declarations;
    struct Less {
        bool operator()(const IR::ID &op1_, const IR::ID &op2_) const;
    };
    typedef std::multimap<IR::ID, IR::StatOrDecl *, Less> ActionInitializations;
    ActionInitializations action_initializations;
    typedef std::multimap<int, IR::StatOrDecl *> StmtInitializations;
    StmtInitializations stmt_initializations;

    class StatementAccumulator {
     private:
        IR::BlockStatement *block = nullptr;

     public:
        /* -- avoid copying */
        StatementAccumulator &operator=(StatementAccumulator &&) = delete;

        void prependStatement(IR::StatOrDecl *stmt_);
        IR::Statement *finish(IR::Statement *stmt_);
    };

    class InjectInitializations : public Transform {
     private:
        const ::P4::ReferenceMap *ref_map;
        ActionInitializations *action_initializations;
        StmtInitializations *stmt_initializations;

     public:
        explicit InjectInitializations(const ::P4::ReferenceMap *ref_map_,
                                       ActionInitializations *action_initializations_,
                                       StmtInitializations *stmt_initializations_);

        /* -- avoid copying */
        InjectInitializations &operator=(InjectInitializations &&) = delete;

        /* -- visitor interface */
        const IR::Node *preorder(IR::MethodCallStatement *stmt_) override;
        const IR::Node *preorder(IR::BlockStatement *stmt_) override;
        const IR::Node *preorder(IR::IfStatement *stmt_) override;
        const IR::Node *preorder(IR::SwitchStatement *stmt_) override;
        const IR::Node *preorder(IR::Statement *stmt_) override;

     private:
        std::vector<const IR::PathExpression *> findActions(IR::MethodCallStatement *stmt_) const;
    };

 public:
    explicit TemporaryVariableInserter(const ::P4::ReferenceMap *ref_map_);

    /* -- avoid copying */
    TemporaryVariableInserter &operator=(TemporaryVariableInserter &&) = delete;

    IR::PathExpression *createTemporaryVariable(const std::string &name_, const IR::Type *var_type_,
                                                IR::Expression *init_expr_,
                                                const IR::Statement *stmt_,
                                                const IR::P4Action *action_);
    void injectIntoControl(IR::P4Control *control_);
};

bool TemporaryVariableInserter::Less::operator()(const IR::ID &op1_, const IR::ID &op2_) const {
    return op1_.name < op2_.name;
}

void TemporaryVariableInserter::StatementAccumulator::prependStatement(IR::StatOrDecl *stmt_) {
    if (block == nullptr) {
        block = new IR::BlockStatement;
    }
    block->components.push_back(stmt_);
}

IR::Statement *TemporaryVariableInserter::StatementAccumulator::finish(IR::Statement *stmt_) {
    if (block == nullptr) return stmt_;
    block->components.push_back(stmt_);
    return block;
}

TemporaryVariableInserter::InjectInitializations::InjectInitializations(
    const ::P4::ReferenceMap *ref_map_,
    TemporaryVariableInserter::ActionInitializations *action_initializations_,
    TemporaryVariableInserter::StmtInitializations *stmt_initializations_)
    : ref_map(ref_map_),
      action_initializations(action_initializations_),
      stmt_initializations(stmt_initializations_) {}

std::vector<const IR::PathExpression *>
TemporaryVariableInserter::InjectInitializations::findActions(
    IR::MethodCallStatement *stmt_) const {
    std::vector<const IR::PathExpression *> action_list_;

    /* -- direct invocation of an action */
    if (const auto *pe_ = stmt_->methodCall->method->to<IR::PathExpression>()) {
        action_list_.push_back(pe_);
        return action_list_;
    }

    /* -- parse activation of a table */

    /* -- <table>.apply() must be a member expression */
    const auto *member_(stmt_->methodCall->method->to<IR::Member>());
    if (member_ == nullptr) return action_list_;

    /* -- name of the method must be 'apply' */
    if (member_->member.name != "apply") return action_list_;

    /* -- get the path expression pointing the table */
    const auto *path_(member_->expr->to<IR::PathExpression>());
    if (path_ == nullptr) return action_list_;

    /* -- find the table declaration */
    const auto *decl_(ref_map->getDeclaration(path_->path));
    if (decl_ == nullptr) return action_list_;
    const auto *table_(decl_->to<IR::P4Table>());
    if (table_ == nullptr) return action_list_;

    /* -- search for the actions property */
    const IR::ActionList *actions_(nullptr);
    for (const auto *property_ : table_->properties->properties) {
        if (property_->name.name == table_->properties->actionsPropertyName) {
            actions_ = property_->value->to<IR::ActionList>();
            break;
        }
    }
    if (actions_ == nullptr) return action_list_;

    /* -- construct list of used actions */
    for (const auto *element_ : actions_->actionList) {
        const auto *mce_(element_->expression->to<IR::MethodCallExpression>());
        if (mce_ == nullptr) continue;
        const auto *pe_(mce_->method->to<IR::PathExpression>());
        if (pe_ == nullptr) continue;
        action_list_.push_back(pe_);
    }

    return action_list_;
}

const IR::Node *TemporaryVariableInserter::InjectInitializations::preorder(
    IR::MethodCallStatement *stmt_) {
    StatementAccumulator acc_;
    for (const auto *action_ : findActions(stmt_)) {
        auto range_(action_initializations->equal_range(action_->path->name));
        for (; range_.first != range_.second; ++range_.first) {
            acc_.prependStatement((*range_.first).second);
        }
    }

    prune();
    return acc_.finish(stmt_);
}

const IR::Node *TemporaryVariableInserter::InjectInitializations::preorder(
    IR::BlockStatement *stmt_) {
    return stmt_;
}

const IR::Node *TemporaryVariableInserter::InjectInitializations::preorder(IR::IfStatement *stmt_) {
    return stmt_;
}

const IR::Node *TemporaryVariableInserter::InjectInitializations::preorder(
    IR::SwitchStatement *stmt_) {
    return stmt_;
}

const IR::Node *TemporaryVariableInserter::InjectInitializations::preorder(IR::Statement *stmt_) {
    StatementAccumulator acc_;
    auto range_(stmt_initializations->equal_range(stmt_->clone_id));
    for (; range_.first != range_.second; ++range_.first) {
        acc_.prependStatement((*range_.first).second);
    }

    prune();
    return acc_.finish(stmt_);
}

TemporaryVariableInserter::TemporaryVariableInserter(const ::P4::ReferenceMap *ref_map_)
    : ref_map(ref_map_) {}

IR::PathExpression *TemporaryVariableInserter::createTemporaryVariable(
    const std::string &name_, const IR::Type *var_type_, IR::Expression *init_expr_,
    const IR::Statement *stmt_, const IR::P4Action *action_) {
    /* -- create variable declaration */
    IR::ID tmp_var_name_(name_);
    declarations.push_back(new IR::Declaration_Variable(tmp_var_name_, var_type_));

    /* -- create the initialization code */
    auto *tmp_var_(new IR::PathExpression(var_type_, new IR::Path(tmp_var_name_)));
    if (init_expr_ != nullptr) {
        auto *assignment_(
            new IR::AssignmentStatement(init_expr_->getSourceInfo(), tmp_var_, init_expr_));
        if (action_ != nullptr) {
            action_initializations.insert(
                ActionInitializations::value_type(action_->name, assignment_));
        } else {
            BUG_CHECK(stmt_, "stmt_ should not be nullptr");
            stmt_initializations.insert(
                StmtInitializations::value_type(stmt_->clone_id, assignment_));
        }
    }

    return tmp_var_;
}

void TemporaryVariableInserter::injectIntoControl(IR::P4Control *control_) {
    if (!declarations.empty()) {
        /* -- insert variable declarations */
        for (auto iter_(declarations.rbegin()); iter_ != declarations.rend(); ++iter_) {
            control_->controlLocals.insert(control_->controlLocals.begin(), *iter_);
        }

        /* -- insert initialization code of the temporary variables */
        control_->body = control_->body
                             ->apply(InjectInitializations(ref_map, &action_initializations,
                                                           &stmt_initializations))
                             ->to<IR::BlockStatement>();
    }

    /* -- reset the inserter state */
    declarations.clear();
    action_initializations.clear();
    stmt_initializations.clear();
}

class ProcessFields : public Transform {
 private:
    ::P4::TypeMap *type_map;
    TemporaryVariableInserter *inserter;

 public:
    explicit ProcessFields(::P4::TypeMap *type_map_, TemporaryVariableInserter *inserter_);

    /* -- visitor interface */
    const IR::Node *preorder(IR::StructExpression *) override;
    const IR::Node *preorder(IR::Constant *) override;
    const IR::Node *preorder(IR::PathExpression *) override;
    const IR::Node *preorder(IR::Member *) override;
    const IR::Node *preorder(IR::TempVar *) override;
    const IR::Node *preorder(IR::Padding *) override;
    const IR::Node *preorder(IR::Neg *) override;
    const IR::Node *preorder(IR::Slice *) override;
    const IR::Node *preorder(IR::Concat *) override;
    const IR::Node *preorder(IR::Expression *expr) override;

 private:
    const IR::Node *skipExpression(IR::Expression *expr);
    const IR::Node *replaceByTemporaryVariable(IR::Expression *expr);
};

ProcessFields::ProcessFields(::P4::TypeMap *type_map_, TemporaryVariableInserter *inserter_)
    : type_map(type_map_), inserter(inserter_) {}

const IR::Node *ProcessFields::preorder(IR::StructExpression *expr) { return expr; }

const IR::Node *ProcessFields::preorder(IR::Constant *constant) { return skipExpression(constant); }

const IR::Node *ProcessFields::preorder(IR::PathExpression *pe) { return skipExpression(pe); }

const IR::Node *ProcessFields::preorder(IR::Member *member) { return skipExpression(member); }

const IR::Node *ProcessFields::preorder(IR::TempVar *tmp) { return skipExpression(tmp); }

const IR::Node *ProcessFields::preorder(IR::Padding *padding) { return skipExpression(padding); }

const IR::Node *ProcessFields::preorder(IR::Neg *neg) { return skipExpression(neg); }

const IR::Node *ProcessFields::preorder(IR::Slice *slice) { return skipExpression(slice); }

const IR::Node *ProcessFields::preorder(IR::Concat *concat) {
    /* -- Concatenation is the same as its fields pushed into
     *    the list. Nest into the concatenation children and
     *    process each of them. */
    return concat;
}

const IR::Node *ProcessFields::preorder(IR::Expression *expr) {
    return replaceByTemporaryVariable(expr);
}

const IR::Node *ProcessFields::skipExpression(IR::Expression *expr) {
    prune();
    return expr;
}

const IR::Node *ProcessFields::replaceByTemporaryVariable(IR::Expression *expr_) {
    static int tempvar_index(0);

    /* -- The hasher list cannot contain expressions, as the hashing engine
     *    accepts inputs just from the PHV input crossbar. In this method
     *    we create a temporary variable, which is filled by the original
     *    expression and which replaces the expression in the field list.
     *
     *    Although some expressions could be implemented by the hashing
     *    matrix (e.g. XOR), the assembler and the dynhash library don't
     *    support this possibility. The hashing algorithm is a matrix
     *    and the expression is a matrix too. However, the dynhash library
     *    doesn't support combining of linear maps together (matrix
     *    multiplication). In addition, the assembler hashing algorithm
     *    directives (crc, xor) expect just a list of fields, not another
     *    hashing algorithm matrix.
     *
     *    The assignments of the local variables is placed into the apply
     *    block of the appropriate control. It's placed just before
     *    a statement which invokes action containing the hashing expression.
     *    The initialization might be duplicated as one action can be used
     *    in more than one tables.
     *
     *    The initialization cannot be placed right before the method call
     *    statement as the compiler is not able to split one action into
     *    a couple of stages. The placement in the apply block forces
     *    the initialization to be moved into a previous stage. */

    /* -- replace the expression by a temporary variable */
    auto *expr_type_(type_map->getType(getOriginal(), true));
    auto *stmt_(findContext<IR::Statement>());
    auto *action_(findContext<IR::P4Action>());
    auto *tmp_var_(
        inserter->createTemporaryVariable("$hash_field_argument" + std::to_string(tempvar_index++),
                                          expr_type_, expr_, stmt_, action_));

    return skipExpression(tmp_var_);
}

class ProcessFieldArgument : public Modifier {
 private:
    ::P4::TypeMap *type_map;
    TemporaryVariableInserter *inserter;

    bool in_vector = false;

 public:
    explicit ProcessFieldArgument(::P4::TypeMap *type_map_, TemporaryVariableInserter *inserter_);

    bool preorder(IR::Node *node) override;
    bool preorder(IR::Vector<IR::Argument> *vector) override;
    bool preorder(IR::Argument *arg) override;
    void postorder(IR::Argument *arg) override;
};

ProcessFieldArgument::ProcessFieldArgument(::P4::TypeMap *type_map_,
                                           TemporaryVariableInserter *inserter_)
    : type_map(type_map_), inserter(inserter_) {}

bool ProcessFieldArgument::preorder(IR::Node *) {
    /* -- don't nest into the content of the arguments */
    return false;
}

bool ProcessFieldArgument::preorder(IR::Vector<IR::Argument> *) {
    BUG_CHECK(!in_vector, "unexpected argument list");
    in_vector = true;
    return true;
}

bool ProcessFieldArgument::preorder(IR::Argument *) {
    BUG_CHECK(in_vector, "unexpected argument");
    return true;
}

void ProcessFieldArgument::postorder(IR::Argument *arg) {
    arg->expression = arg->expression->apply(ProcessFields(type_map, inserter), getContext());
}

class PickUpHasher : public Transform {
 private:
    ::P4::TypeMap *type_map;
    TemporaryVariableInserter inserter;

 public:
    explicit PickUpHasher(::P4::ReferenceMap *ref_map_, ::P4::TypeMap *type_map_);

    /* -- avoid copying */
    PickUpHasher &operator=(PickUpHasher &&) = delete;

    const IR::Node *preorder(IR::MethodCallExpression *mc) override;
    const IR::Node *postorder(IR::P4Control *control_) override;
};

PickUpHasher::PickUpHasher(::P4::ReferenceMap *ref_map_, ::P4::TypeMap *type_map_)
    : type_map(type_map_), inserter(ref_map_) {}

const IR::Node *PickUpHasher::preorder(IR::MethodCallExpression *mc) {
    if (!NormalizeHashList::isHasher(mc)) return mc;

    /* -- find all hash field list items which should be replaced by a temporary variable */
    mc->arguments = mc->arguments->apply(ProcessFieldArgument(type_map, &inserter), getContext());

    /* -- Don't nest, we have already done what we wanted to do. */
    prune();
    return mc;
}

const IR::Node *PickUpHasher::postorder(IR::P4Control *control_) {
    inserter.injectIntoControl(control_);
    return control_;
}

}  // namespace

NormalizeHashList::NormalizeHashList(::P4::ReferenceMap *ref_map_, ::P4::TypeMap *type_map_,
                                     ::P4::TypeChecking *type_checking_) {
    addPasses({type_checking_, new PickUpHasher(ref_map_, type_map_),
               /* -- new variables are created, clear the map and move local declarations */
               new P4::ClearTypeMap(type_map_), type_checking_});
}

bool NormalizeHashList::isHasher(const IR::MethodCallExpression *mc) {
    const auto *member_method(mc->method->to<IR::Member>());
    if (member_method == nullptr) return false;
    const auto *method_this(member_method->expr->to<IR::PathExpression>());
    if (method_this == nullptr) return false;
    const auto *this_type(method_this->type->to<IR::Type_SpecializedCanonical>());
    if (this_type == nullptr) return false;
    const auto *this_base_type(this_type->baseType->to<IR::Type_Extern>());
    if (this_base_type == nullptr) return false;
    if (this_base_type->name.name != "Hash") return false;

    /* -- the extern type is OK, now check method name */
    if (member_method->member.name != "get") return false;

    return true;
}
