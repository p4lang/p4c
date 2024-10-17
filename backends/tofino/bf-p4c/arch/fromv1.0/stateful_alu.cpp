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

#include <vector>
#include <utility>

#include "stateful_alu.h"
#include "frontends/p4-14/typecheck.h"
#include "frontends/p4/strengthReduction.h"
#include "frontends/common/constantFolding.h"
#include "lib/bitops.h"
#include "programStructure.h"

P4V1::StatefulAluConverter::StatefulAluConverter() {
    addConverter("stateful_alu"_cs, this);
}

const IR::Type_Extern *P4V1::StatefulAluConverter::convertExternType(
    P4V1::ProgramStructure *structure, const IR::Type_Extern *, cstring) {
    if (!has_stateful_alu) {
        has_stateful_alu = true;
        if (use_v1model()) {
            auto &options = BackendOptions();
            std::stringstream versionArg;
            versionArg << "-DV1MODEL_VERSION=" << options.v1ModelVersion;
            structure->include("tofino/stateful_alu.p4"_cs, versionArg.str());
        }
    }
    return nullptr;
}

/**
 * @brief This inspector runs the expression analysis to search
 * for the predicate that cannot happen. For example:
 * - expr && !expr
 * - !expr && expr
 * - expr && expr2 && !expr
 *
 * @warning Relation operator is taken as atomic value and it shouldn't contain any relation
 * sub-expressions.
 */
class ConstantLogicValue : public Inspector {
    /**
     * @brief Collect all relation operator atomic expressions
     * (e.g., there is no other relation operator in subtree)
     */
    struct CollectRelationAtoms : public Inspector {
        std::vector<const IR::Operation_Relation*> m_rel_atoms;
        // Flag which means that all nodes were collected without any error
        // bool m_collection_ok = true;

        bool preorder(const IR::Operation_Relation *op) override {
            // Check if we have any relation operator as a sub-expression
            for (auto node : {op->left, op->right}) {
                bool has_relation = false;
                forAllMatching<IR::Operation_Relation>(node,
                    [&has_relation](const IR::Operation_Relation *){ has_relation = true; });
                if (has_relation) {
                   LOG4("We need to continue into the expression");
                   return true;
                }
            }

            // Everything seems fine insert the node if it is unique, the operator is inserted
            // if needed
            auto result = std::find_if(m_rel_atoms.begin(), m_rel_atoms.end(),
                [&](const IR::Operation_Relation *node) {
                    return node->equiv(*op);
            });
            if (result == m_rel_atoms.end()) {
                // Check that we have a format required for the analysis
                LOG4("Pushing the node into the collection: " << op);
                m_rel_atoms.push_back(op);
            }

            return false;
        }
    };

    // Data vector configuration
    using ExpressionConfig = std::pair<const IR::Expression*, bool>;

    /**
     * @brief Replace all relation operators with a passed
     * value vector.
     */
    class RewriteTerms : public Transform {
        const std::vector<ExpressionConfig>& m_config;

     public:
        IR::Node* preorder(IR::Operation_Relation *op) override {
            // Check if the operation exists and copy the configuration if yes
            LOG4("Trying to replace: " << op);
            auto result = std::find_if(m_config.begin(), m_config.end(),
                [&](const ExpressionConfig &rel) {
                    return rel.first->equiv(*op);
                });
            // Return the original node if we don't have a configuration
            // for this expression
            if (result == m_config.end()) {
                return op;
            } else {
                auto ret = new IR::BoolLiteral((*result).second);
                LOG4("Rewriting the node " << op << " with " << ret);
                return ret;
            }
        }

        explicit RewriteTerms(const std::vector<ExpressionConfig> &conf) :
            m_config(conf) {}
    };

    // Computed flags for the next analysis
    bool m_always_false;
    bool m_always_true;

    // Remember the analyzed subtree
    const IR::Expression *m_expression;

    void compute_flags(const CollectRelationAtoms &collect_exprs) {
        // Now, we need to try all possibilities of relation
        // operator values and setup all corresponding flags
        // of this class.
        //
        // Identify the maximum number of iterations to probe all possibilities
        if (collect_exprs.m_rel_atoms.size() == 0) return;

        bool is_always_false = true;
        bool is_always_true  = true;

        unsigned max_iter = (1 << collect_exprs.m_rel_atoms.size()) - 1;
        for (unsigned it = 0; it <= max_iter; it++) {
            // TODO: It will be better to implement this as iterator over the
            // binary vector of predefined values
            // 1] Prepare the replacement configuration
            unsigned tmp_conf = it;
            std::vector<ExpressionConfig> ep;
            std::stringstream ss;
            ss << "Creating the following expression configuration:" << std::endl;
            for (unsigned bit = 0; bit < collect_exprs.m_rel_atoms.size(); bit ++) {
                auto econf = std::make_pair(collect_exprs.m_rel_atoms[bit], tmp_conf & 0x1);
                ss << "\t* " << econf.first << " =====> " << econf.second << std::endl;
                ep.push_back(econf);
                tmp_conf <<= 1;
            }
            if (LOGGING(4)) LOG4(ss.str());

            // 2] Rewrite the copy of the tree
            RewriteTerms rwt(ep);
            auto rew_expr = m_expression->apply(RewriteTerms(ep));
            LOG4("Running the strength analysis of following expression: " << rew_expr);

            // 3] Do the strength reduction, check the result and aggregate
            // the flag.
            PassRepeated reduc_pass({
                new P4::DoStrengthReduction(),
                new P4::DoConstantFolding(nullptr, nullptr, true),
            });
            rew_expr = rew_expr->apply(reduc_pass);
            LOG4("Strength reduction result: " << rew_expr);
            // The result can be an expression which needs to be
            // analyzed.
            if (!rew_expr->is<IR::BoolLiteral>()) {
                // We cannot distinguish the real value
                is_always_false = false;
                is_always_true  = false;
                LOG4("Cannot distinguish the constant value (true/false)");
                break;
            }
            auto bl = rew_expr->to<IR::BoolLiteral>();
            is_always_false &= !bl->value;
            is_always_true  &= bl->value;
        }

        // Store computed flags
        m_always_false = is_always_false;
        m_always_true  = is_always_true;
        LOG4("Analysis result always_false=" << m_always_false << ", always_true=" <<
            m_always_true);
    }

 public:
    explicit ConstantLogicValue(const IR::Expression *expr) :
        m_always_false(false), m_always_true(false), m_expression(expr) {
        LOG4("Starting the analysis of: " << expr);
        CollectRelationAtoms collect_exprs;
        m_expression->apply(collect_exprs);

        // Check if is safe to run the analysis
        // if (collect_exprs.m_collection_ok)
        compute_flags(collect_exprs);
    }

    bool is_false() const {
        return m_always_false;
    }

    bool is_true() const {
        return m_always_true;
    }
};

class CreateSaluApplyFunction : public Inspector {
    // These annotations will be put on the RegisterAction instance that will be
    // created to contain the apply function being created by this inspector
    IR::Annotations *annots = nullptr;
    P4V1::ProgramStructure *structure;
    const IR::Type *rtype;
    const IR::Type::Bits *utype;
    cstring math_unit_name;
    IR::BlockStatement *body;
    IR::Function *apply = nullptr;
    const IR::Expression *cond_lo = nullptr;
    const IR::Expression *cond_hi = nullptr;
    const IR::Expression *math_input = nullptr;
    const IR::Expression *pred = nullptr;
    // Each expression there has a predicate on corresponding index
    // where index is of type expr_index_t.
    // The NUM_EXPR_INDEX_T --> number of available slots in SALU
    static const unsigned NUM_EXPR_INDEX_T  = 5;
    const IR::Expression *expr[NUM_EXPR_INDEX_T]       = { nullptr };
    const IR::Expression *expr_pred[NUM_EXPR_INDEX_T]  = { nullptr };
    const IR::Expression *expr_dest[NUM_EXPR_INDEX_T]  = { nullptr };
    int                   expr_idx[NUM_EXPR_INDEX_T]   = { 0 };
    Util::SourceInfo      expr_src_info[NUM_EXPR_INDEX_T];
    Util::SourceInfo      expr_pred_src_info[NUM_EXPR_INDEX_T];
    const IR::Statement *output = nullptr;
    const Util::SourceInfo *applyLoc = nullptr;
    enum expr_index_t { LO1, LO2, HI1, HI2, OUT, UNUSED } expr_index = UNUSED;
    bool saturating = false;
    bool convert_to_saturating = false;
    bool have_output = false;
    bool defer_out = false;
    bool cmpl_out = false;
    bool need_alu_hi = false;
    // bool alu_output_used[2] = { false, false };
    PassManager rewrite;

    IR::Expression *makeRegFieldMember(IR::Expression *e, int idx) {
        if (auto st = rtype->to<IR::Type_Struct>()) {
            for (auto f : st->fields)
                if (--idx < 0)
                    return new IR::Member(e, f->name);
            idx = 1; }
        if (idx > 0) {
            need_alu_hi = true;
            e = new IR::PathExpression(utype, new IR::Path("alu_hi")); }
        return e; }

    // cast an expression to a type if necessary
    const IR::Expression *castTo(const IR::Type *type, const IR::Expression *e) {
        if (e->type != type) {
            auto t1 = e->type->to<IR::Type::Bits>();
            auto t2 = type->to<IR::Type::Bits>();
            if (t1 && t2 && t1->size != t2->size && t1->isSigned != t2->isSigned) {
                /* P4_16 does not allow changing both size and signedness with a single cast,
                 * so we need two, changing size first, then signedness */
                e = new IR::Cast(IR::Type::Bits::get(t2->size, t1->isSigned), e); }
            e = new IR::Cast(type, e); }
        return e;
    }

    class RewriteExpr : public Transform {
        CreateSaluApplyFunction &self;
        const IR::Expression *preorder(IR::Cast *c) override {
            if (c->expr->is<IR::AttribLocal>()) return c->expr;
            return c; };
        const IR::Expression *preorder(IR::Constant *c) override {
            // re-infer the types of all constants from context, as it may have changed
            // FIXME -- probably only want this for constants that did not have an explicit
            // FIXME -- type originally, but we have no good way of knowing.
            // If we fix P4_14 typechecking to understand externs, need for this goes away
            c->type = IR::Type_InfInt::get();
            return c; }
        const IR::Expression *postorder(IR::Add *e) override {
            if (self.convert_to_saturating)
                return new IR::AddSat(e->srcInfo, e->type, e->left, e->right);
            return e; }
        const IR::Expression *postorder(IR::Sub *e) override {
            if (self.convert_to_saturating)
                return new IR::SubSat(e->srcInfo, e->type, e->left, e->right);
            return e; }
        const IR::Expression *postorder(IR::Neg *e) override {
            if (e->type->is<IR::Type_InfInt>())
                return new IR::Neg(e->srcInfo, e->expr->type, e->expr);
            return e;
        }
        const IR::Expression *postorder(IR::AttribLocal *attr) override {
            int idx = 0;
            IR::Path *var = nullptr;
            if (attr->name == "condition_lo") {
                if (self.cond_lo == nullptr) {
                    error("condition_lo used and not specified %s", *self.applyLoc);
                    return new IR::BoolLiteral(false);
                }
                return self.cond_lo;
            } else if (attr->name == "condition_hi") {
                if (self.cond_hi == nullptr) {
                    error("condition_hi used and not specified %s", *self.applyLoc);
                    return new IR::BoolLiteral(false);
                }
                return self.cond_hi;
            } else if (attr->name == "register_lo") {
                var = new IR::Path("in_value");
                idx = 0;
            } else if (attr->name == "register_hi") {
                var = new IR::Path("in_value");
                idx = 1;
            } else if (attr->name == "alu_lo") {
                // For complement we use the 'in_value' or input value as the
                // ordering of instructions may change the 'value'
                // E.g. set_bitc creates an action,
                // value = 1;
                // rv = ~value;
                // Due to ordering this gets transformed to
                // value = 1;
                // rv = 0; <- which is incorrect
                // By using in_value, we get,
                // value = 1;
                // rv = ~in_value;
                auto valStr = self.cmpl_out ? "in_value" : "value";
                var = new IR::Path(valStr);
                idx = 0;
                self.defer_out = true;
            } else if (attr->name == "alu_hi") {
                auto valStr = self.cmpl_out ? "in_value" : "value";
                var = new IR::Path(valStr);
                idx = 1;
                self.defer_out = true;
            } else if (attr->name == "set_bit" || attr->name == "set_bitc") {
                if (self.utype->width_bits() != 1)
                    error("%s only allowed in 1 bit registers", attr->name);
                return new IR::Constant(self.utype, 1);
            } else if (attr->name == "clr_bit" || attr->name == "clr_bitc") {
                if (self.utype->width_bits() != 1)
                    error("%s only allowed in 1 bit registers", attr->name);
                return new IR::Constant(self.utype, 0);
            } else if (attr->name == "read_bit" || attr->name == "read_bitc") {
                if (self.utype->width_bits() != 1)
                    error("%s only allowed in 1 bit registers", attr->name);
                idx = 0;
                var = new IR::Path("in_value");
            } else if (attr->name == "math_unit") {
                auto *mu = new IR::PathExpression(IR::ID(attr->srcInfo, self.math_unit_name));
                return new IR::MethodCallExpression(attr->srcInfo,
                        new IR::Member(mu, "execute"), { new IR::Argument(self.math_input) });
            } else if (attr->name == "predicate") {
                auto *args = new IR::Vector<IR::Argument>;
                if (self.cond_lo)
                    args->push_back(new IR::Argument(self.cond_lo));
                else if (self.cond_hi)
                    args->push_back(new IR::Argument(new IR::BoolLiteral(false)));
                if (self.cond_hi)
                    args->push_back(new IR::Argument(self.cond_hi));
                return new IR::MethodCallExpression(attr->srcInfo,
                    new IR::Member(new IR::This, "predicate"), args);
            } else if (attr->name == "combined_predicate") {
                // combined_predicate is an 1-bit output equal to any boolean function of
                // the predicate results.  We translate to a conditional setting of the
                // output to a constant 1, with the boolean function as the condition.
                if (!self.pred) {
                    /* documentation suggests that the predicate output will always be
                     * "condition_hi || condition_lo", but we only use that if the user
                     * does not explicitly set "output_predicate:" */
                    if (self.cond_hi && self.cond_lo)
                        self.pred = new IR::LOr(self.cond_hi, self.cond_lo);
                    else if (self.cond_hi)
                        self.pred = self.cond_hi;
                    else if (self.cond_lo)
                        self.pred = self.cond_lo;
                    else
                        self.pred = new IR::BoolLiteral(false); }
                return new IR::Constant(self.utype, 1);
            } else {
                error("Unrecognized attribute %s", attr);
                return attr; }
            return self.makeRegFieldMember(
                    new IR::PathExpression(attr->srcInfo, self.rtype, var), idx); }
        const IR::Expression *postorder(IR::Primitive *prim) override {
            if (prim->name == "salu_min") prim->name = "min"_cs;
            if (prim->name == "salu_max") prim->name = "max"_cs;
            return prim; }

     public:
        explicit RewriteExpr(CreateSaluApplyFunction &self) : self(self) {}
    };

    bool preorder(const IR::Property *prop) {
        LOG4("CreateSaluApply visiting prop " << prop);
        bool predicate = false;
        int idx = 0;
        if (prop->name == "condition_hi") {
            applyLoc = &prop->value->srcInfo;
            cond_hi = prop->value->to<IR::ExpressionValue>()->expression->apply(rewrite);
            cond_hi = castTo(IR::Type::Boolean::get(), cond_hi);
            return false;
        } else if (prop->name == "condition_lo") {
            applyLoc = &prop->value->srcInfo;
            cond_lo = prop->value->to<IR::ExpressionValue>()->expression->apply(rewrite);
            cond_lo = castTo(IR::Type::Boolean::get(), cond_lo);
            return false;
        } else if (prop->name == "math_unit_input") {
            applyLoc = &prop->value->srcInfo;
            math_input = prop->value->to<IR::ExpressionValue>()->expression->apply(rewrite);
            math_input = castTo(utype, math_input);
            return false;
        } else if ((prop->name == "initial_register_lo_value")
                || (prop->name == "initial_register_hi_value")) {
            if (auto ev = prop->value->to<IR::ExpressionValue>()) {
                if (auto k = ev->expression->to<IR::Constant>()) {
                    annots->addAnnotation(prop->name.toString(), ev->expression);
                    LOG5("adding annotation '" << prop->name << "' with value " << k->asInt());
                    return false; } }
            error("%s: %s must be a constant", prop->value->srcInfo, prop->name);
            return false;
        } else if (prop->name == "update_lo_1_predicate") {
            expr_index = LO1;
            predicate = true;
        } else if (prop->name == "update_lo_2_predicate") {
            expr_index = LO2;
            predicate = true;
        } else if (prop->name == "update_hi_1_predicate") {
            expr_index = HI1;
            predicate = true;
        } else if (prop->name == "update_hi_2_predicate") {
            expr_index = HI2;
            predicate = true;
        } else if (prop->name == "output_predicate") {
            expr_index = OUT;
            predicate = true;
        } else if (prop->name == "update_lo_1_value") {
            if (expr_index != LO1) pred = nullptr;
            expr_index = LO1;
            idx = 0;
        } else if (prop->name == "update_lo_2_value") {
            if (expr_index != LO2) pred = nullptr;
            expr_index = LO2;
            idx = 0;
        } else if (prop->name == "update_hi_1_value") {
            if (expr_index != HI1) pred = nullptr;
            expr_index = HI1;
            idx = 1;
        } else if (prop->name == "update_hi_2_value") {
            if (expr_index != HI2) pred = nullptr;
            expr_index = HI2;
            idx = 1;
        } else if (prop->name == "output_value") {
            have_output = true;
            if (expr_index != OUT) pred = nullptr;
            expr_index = OUT;
            idx = -1;
        } else {
            return false; }

        applyLoc = &prop->value->srcInfo;
        convert_to_saturating = saturating && !predicate;
        auto e = prop->value->to<IR::ExpressionValue>()->expression->apply(rewrite);
        convert_to_saturating = false;

        // Store data required during the end_apply SALU body creation if we have
        // a non-empty expression
        if (!e) return false;
        if (predicate) {
            expr_pred[expr_index] = castTo(IR::Type::Boolean::get(), e);
            expr_pred_src_info[expr_index] = prop->srcInfo;
            return false;
        }

        // Prepare the destination expression based on the detected
        // property.
        IR::Expression *dest = new IR::PathExpression(utype,
            new IR::Path(idx < 0 ? "rv" : "value"));
        if (idx >= 0) {
            dest = makeRegFieldMember(dest, idx);
        } else if (cmpl_out) {
            e = new IR::Cmpl(e);
        }

        // Remember everything important for the next iteration
        expr_dest[expr_index]     = dest;
        expr[expr_index]          = e;
        expr_idx[expr_index]      = idx;
        expr_src_info[expr_index] = prop->srcInfo;
        return false; }

    /**
     * @brief Create the standard assignment statement for captured
     * data from the property phase
     *
     * @param alu_idx ALU IDX where to start the analysis
     * @return const IR::Statement* of the IR::AssignmentStatement, the statement
     * can be also wrapped in the IF statement when needed.
     */
    const IR::Statement* standard_assignment(int alu_idx) {
        const IR::Statement *instr = structure->assign(expr_src_info[alu_idx],
                                expr_dest[alu_idx], expr[alu_idx], utype);
        LOG2("adding " << instr << " with pred " << expr_pred[alu_idx]);
        // Add the IF condition in a case that we have a not null predicate
        if (expr_pred[alu_idx]) {
            instr = new IR::IfStatement(expr_pred[alu_idx], instr, nullptr);
        }

        return instr;
    }

    /**
     * @brief We want to generate a following shape of the code
     *  if (P1) {
     *      res = E1 | E2;
     *  } else {
     *      res = E2;
     *  }
     * 
     * @param alu_idx ALU IDX where we are starting the analysis
     * @return const IR::Statement* instance of created if-else statement
     */
    const IR::Statement* merge_assigment_with_if(int alu_idx) {
        // Prepare individual parts of the expression above, assignments, predicate
        // and false branch. Individual parts of captured data depends on not null
        // predicate occupancy
        auto assig1 = structure->assign(expr_src_info[alu_idx], expr_dest[alu_idx],
            expr[alu_idx], utype);
        auto assig2 = structure->assign(expr_src_info[alu_idx+1], expr_dest[alu_idx+1],
            expr[alu_idx+1], utype);
        auto pred = expr_pred[alu_idx] ? expr_pred[alu_idx] : expr_pred[alu_idx+1];
        auto false_branch = expr_pred[alu_idx] ? assig2 : assig1;

        auto ret_if = new IR::IfStatement(pred,
            // res = E1 | E2
            new IR::AssignmentStatement(assig1->left,
                new IR::BOr(assig1->right, assig2->right)),
            // res = E2
            false_branch);

        LOG2("Adding the if statement " << ret_if);
        return ret_if;
    }

    /**
     * @brief Create the if statement for captured data from the
     * property phase
     *
     * @param alu_idx ALU IDX where to start the analysis
     * @return const IR::Statement* instance of created if-else statement
     */
    const IR::Statement* merge_if_statements(int alu_idx) {
        auto pred1 = expr_pred[alu_idx];
        auto pred2 = expr_pred[alu_idx+1];
        auto assig1 = structure->assign(expr_src_info[alu_idx], expr_dest[alu_idx],
            expr[alu_idx], utype);
        auto assig2 = structure->assign(expr_src_info[alu_idx+1], expr_dest[alu_idx+1],
            expr[alu_idx+1], utype);

        const IR::Statement *ret_stmt;
        if (pred1->equiv(*pred2)) {
            // Both predicates are same and we can build an expression if the form of:
            // if (pred) {
            //    value = expr1 | expr2;
            // }
            ret_stmt = new IR::IfStatement(pred1,
                new IR::AssignmentStatement(assig1->left,
                    new IR::BOr(assig1->right, assig2->right)),
                nullptr);
        } else {
            // Both predicates are different and we need to generate a more complicated
            // structure of IF statements which reflects the SALU behavior:
            // if (pred1 && pred2) dst = expr1 | expr2;
            // else if (pred1)  dst = expr1;
            // else if (pred2)  dst = expr2;

            // if (pred1) ...
            // else if(pred2) ...
            ret_stmt = new IR::IfStatement(pred1, assig1,
                new IR::IfStatement(pred2, assig2, nullptr));

            // Run the always false analysis to check if we can insert the
            // AND-ed condition here
            auto and_pred = new IR::LAnd(pred1, pred2);
            ConstantLogicValue always_false(and_pred);
            if (!always_false.is_false()) {
                    // if (pred1 && pred2 ) {
                ret_stmt = new IR::IfStatement(and_pred,
                    // dst = expr1 | expr2; }
                    new IR::AssignmentStatement(assig1->left,
                        new IR::BOr(assig1->right, assig2->right)),
                    ret_stmt);
            }
        }

        LOG2("Adding the if statement " << ret_stmt);
        return ret_stmt;
    }

    /**
     * @brief Emit output instruction for the SALU unit
     */
    void emit_output() {
        auto out_inst = standard_assignment(OUT);
        body->components.push_back(out_inst);
    }

    /**
     * @brief Check the predicate and return error message
     * 
     * @warning The code assumes that nullptr value of the expression was checked from callee
     * @param alu_idx ALU Index to check
     */
    void check_predicate_for_null(int alu_idx) const {
        if (!expr_pred[alu_idx]) return;

        error("Corresponding expression for the predicate wasn't found! %s",
            expr_pred_src_info[alu_idx]);
    }

    /**
     * @brief This function takes all expressions & predicates captured during the
     * property analysis and crafts the target SALU body.
     */
    void emit_salu_body() {
        // The for cycle is processing the body in tuples where we are
        // always starting from lower indexes
        // Check if we need to deffer the output
        if (!defer_out && have_output) emit_output();

        for (auto alu_idx : {LO1, HI1}) {
            // We have following possibilities:
            // 0] Both expressions are null
            // 1] LO1/HI1 isn't null but LO2/HI2 is
            // 2] LO2/HI2 isn't null but LO1/HI1 is
            // 3] Both expression are not null
            if (!expr[alu_idx] && !expr[alu_idx+1]) {
                  check_predicate_for_null(alu_idx);
                  check_predicate_for_null(alu_idx+1);
                  continue;
            }

            if (expr[alu_idx] && !expr[alu_idx+1]) {
                check_predicate_for_null(alu_idx+1);
                body->components.push_back(standard_assignment(alu_idx));
                continue;
            }
            if (!expr[alu_idx] && expr[alu_idx+1]) {
                check_predicate_for_null(alu_idx);
                body->components.push_back(standard_assignment(alu_idx+1));
                continue;
            }

            // Both expressions are there, the next behavior is based on predicates
            if (!expr_pred[alu_idx] && !expr_pred[alu_idx+1]) {
                // Predicates aren't available and we can merge
                // expressions together
                auto assig1 = standard_assignment(alu_idx)->to<IR::AssignmentStatement>();
                auto assig2 = standard_assignment(alu_idx+1)->to<IR::AssignmentStatement>();
                BUG_CHECK(assig1 && assig2,
                    "IR::AssignmentStatement statements are expected here!");
                auto instr = new IR::AssignmentStatement(assig1->left,
                    new IR::BOr(assig1->right, assig2->right));
                LOG2("Adding the merged assignment " << instr);
                body->components.push_back(instr);
                continue;
            }
            if ((expr_pred[alu_idx] && !expr_pred[alu_idx+1]) ||
                (!expr_pred[alu_idx] && expr_pred[alu_idx+1])) {
                // Only one predicate is there and we need to emit a specialized
                // IF statement (see the method for details).
                body->push_back(merge_assigment_with_if(alu_idx));
                continue;
            }

            // Both predicates are there and we can merge them together
            body->components.push_back(merge_if_statements(alu_idx));
        }
        // Generate the output instruction if required
        if (defer_out && have_output)
            emit_output();
    }

 public:
    CreateSaluApplyFunction(IR::Annotations *annots, P4V1::ProgramStructure *s,
                            const IR::Type *rtype, const IR::Type::Bits *utype, cstring mu,
                            bool saturating)
        : annots(annots), structure(s), rtype(rtype), utype(utype), math_unit_name(mu),
          saturating(saturating),
          rewrite({ new RewriteExpr(*this), new TypeCheck }) {
        body = new IR::BlockStatement({
            new IR::Declaration_Variable("in_value"_cs, rtype),
            new IR::AssignmentStatement(new IR::PathExpression("in_value"),
                                        new IR::PathExpression("value")) });
        if (auto st = rtype->to<IR::Type_StructLike>())
            rtype = new IR::Type_Name(st->name); }
    void end_apply(const IR::Node *) {
        // Emit helping alu_hi & rv when needed
        if (need_alu_hi)
            body->components.insert(body->components.begin(),
                    new IR::Declaration_Variable("alu_hi", utype, new IR::Constant(utype, 0)));
        auto apply_params = new IR::ParameterList({
                     new IR::Parameter("value", IR::Direction::InOut, rtype) });
        if (have_output) {
            body->components.insert(body->components.begin(),
                new IR::AssignmentStatement(new IR::PathExpression("rv"),
                                            new IR::Constant(utype, 0)));
            apply_params->push_back(new IR::Parameter("rv", IR::Direction::Out, utype)); }
        // Emit body and prepare the apply method
        emit_salu_body();
        apply = new IR::Function("apply",
                new IR::Type_Method(IR::Type_Void::get(), apply_params, "apply"_cs), body);
    }
    static const IR::Function *create(IR::Annotations *annots, P4V1::ProgramStructure *structure,
                const IR::Declaration_Instance *ext, const IR::Type *rtype,
                const IR::Type::Bits *utype, cstring math_unit_name = cstring(),
                bool saturating = false) {
        CreateSaluApplyFunction create_apply(annots, structure, rtype, utype,
                                             math_unit_name, saturating);
        // need a separate traversal here as "update" will be visited after "output"
        forAllMatching<IR::AttribLocal>(&ext->properties, [&](const IR::AttribLocal *attr) {
            if (attr->name.name.endsWith("_bitc")) { create_apply.cmpl_out = true; } });
        ext->apply(create_apply);
        return create_apply.apply; }
};

static bool usesRegHi(const IR::Declaration_Instance *salu) {
    struct scanVisitor : public Inspector {
        bool result = false;
        bool preorder(const IR::AttribLocal *attr) override {
            if (attr->name == "register_hi")
                result = true;
            return !result; }
        bool preorder(const IR::Expression *) override { return !result; }
    } scan;

    salu->properties.apply(scan);
    return scan.result;
}

class CreateMathUnit : public Inspector {
    cstring name;
    const IR::Type::Bits *utype;
    IR::Declaration_Instance *unit = nullptr;
    bool have_unit = false;
    const IR::BoolLiteral *exp_invert = nullptr;
    const IR::Constant *exp_shift = nullptr, *output_scale = nullptr;
    const IR::ExpressionListValue *table = nullptr;

    bool preorder(const IR::Property *prop) {
        if (prop->name == "math_unit_exponent_invert") {
            have_unit = true;
            if (auto ev = prop->value->to<IR::ExpressionValue>())
                if ((exp_invert = ev->expression->to<IR::BoolLiteral>()))
                    return false;
            error("%s: %s must be a constant", prop->value->srcInfo, prop->name);
        } else if (prop->name == "math_unit_exponent_shift") {
            have_unit = true;
            if (auto ev = prop->value->to<IR::ExpressionValue>())
                if ((exp_shift = ev->expression->to<IR::Constant>()))
                    return false;
            error("%s: %s must be a constant", prop->value->srcInfo, prop->name);
        } else if (prop->name == "math_unit_input") {
            have_unit = true;
        } else if (prop->name == "math_unit_lookup_table") {
            have_unit = true;
            if (!(table = prop->value->to<IR::ExpressionListValue>()))
                error("%s: %s must be a list", prop->value->srcInfo, prop->name);
        } else if (prop->name == "math_unit_output_scale") {
            have_unit = true;
            if (auto ev = prop->value->to<IR::ExpressionValue>())
                if ((output_scale = ev->expression->to<IR::Constant>()))
                    return false;
            error("%s: %s must be a constant", prop->value->srcInfo, prop->name); }
        return false;
    }

 public:
    CreateMathUnit(cstring n, const IR::Type::Bits *utype) : name(n), utype(utype) {}
    void end_apply(const IR::Node *) {
        if (!have_unit) {
            unit = nullptr;
            return; }
        if (!exp_invert) exp_invert = new IR::BoolLiteral(false);
        if (!exp_shift) exp_shift = new IR::Constant(0);
        if (!output_scale) output_scale = new IR::Constant(0);
        if (!table) table = new IR::ExpressionListValue({});
        /// TODO: remove when v1model is retired
        IR::Type* mutype;
        if (P4V1::use_v1model()) {
            auto *tuple_type = new IR::Type_Tuple;
            for (int i = table->expressions.size(); i > 0; --i)
                tuple_type->components.push_back(utype);
            mutype = new IR::Type_Specialized(new IR::Type_Name("math_unit"),
                    new IR::Vector<IR::Type>({ utype, tuple_type }));
        } else {
            mutype = new IR::Type_Specialized(new IR::Type_Name("MathUnit"),
                    new IR::Vector<IR::Type>({ utype }));
        }
        auto *ctor_args = new IR::Vector<IR::Argument>({
            new IR::Argument(exp_invert),
            new IR::Argument(exp_shift),
            new IR::Argument(output_scale),
            new IR::Argument(new IR::ListExpression(table->expressions))
        });
        auto* externalName = new IR::StringLiteral(IR::ID("." + name));
        auto* annotations = new IR::Annotations({
                new IR::Annotation(IR::ID("name"), { externalName })
                });
        unit = new IR::Declaration_Instance(name, annotations, mutype, ctor_args);
    }
    static const IR::Declaration_Instance *create(P4V1::ProgramStructure *structure,
                const IR::Declaration_Instance *ext, const IR::Type::Bits *utype) {
        LOG3("creating math unit " << ext);
        CreateMathUnit create_math(structure->makeUniqueName(ext->name + "_math_unit"), utype);
        ext->apply(create_math);
        return create_math.unit; }
};

/* FIXME -- still need to deal with the following stateful_alu properties:
        initial_register_hi_value initial_register_lo_value
        reduction_or_group
        selector_binding
        stateful_logging_mode
*/

// FIXME -- this should be a method of IR::IndexedVector, or better yet, have a
// FIXME -- replace method that doesn't need an iterator.
IR::IndexedVector<IR::Node>::iterator find_in_scope(
        IR::Vector<IR::Node> *scope, cstring name) {
    for (auto it = scope->begin(); it != scope->end(); ++it) {
        if (auto decl = (*it)->to<IR::Declaration>()) {
            if (decl->name == name) {
                return it; } }
    }
    BUG_CHECK("%s not in scope", name);
    return scope->end();
}

P4V1::StatefulAluConverter::reg_info P4V1::StatefulAluConverter::getRegInfo(
        P4V1::ProgramStructure *structure, const IR::Declaration_Instance *ext,
        IR::Vector<IR::Node> *scope) {
    reg_info rv;
    if (auto rp = ext->properties.get<IR::Property>("reg"_cs)) {
        auto rpv = rp->value->to<IR::ExpressionValue>();
        auto gref = rpv ? rpv->expression->to<IR::GlobalRef>() : nullptr;
        if ((rv.reg = gref ? gref->obj->to<IR::Register>() : nullptr)) {
            if (cache.count(rv.reg)) return cache.at(rv.reg);
            if (rv.reg->layout) {
                rv.rtype = structure->types.get(rv.reg->layout);
                if (!rv.rtype) {
                    error("No type named %s", rv.reg->layout);
                } else if (auto st = rv.rtype->to<IR::Type_Struct>()) {
                    auto nfields = st->fields.size();
                    bool sameTypes = false;
                    if (nfields > 0)
                        rv.utype = st->fields.at(0)->type->to<IR::Type::Bits>();
                    if (nfields > 1) {
                        auto* secondType = st->fields.at(1)->type;
                        if (rv.utype && secondType)
                            sameTypes = rv.utype->equiv(*secondType);
                    }
                    if (nfields < 1 || nfields > 2 ||
                        !rv.utype ||
                        (nfields > 1 && !sameTypes))
                        rv.utype = nullptr;
                    if (!rv.utype)
                        error("%s not a valid register layout for stateful_alu", rv.reg->layout);
                } else {
                    error("%s is not a struct type", rv.reg->layout);
                }
            } else if (rv.reg->width <= 64) {
                int width = 1 << ceil_log2(rv.reg->width);
                if (width > 1 && width < 8) width = 8;
                if (width > 32 || usesRegHi(ext)) {
                    rv.utype = IR::Type::Bits::get(width/2, rv.reg->signed_);
                    cstring rtype_name = structure->makeUniqueName(ext->name + "_layout"_cs);
                    rv.rtype = new IR::Type_Struct(IR::ID(rtype_name), {
                        new IR::StructField("lo", rv.utype),
                        new IR::StructField("hi", rv.utype) });
                    auto iter = find_in_scope(scope, structure->registers.get(rv.reg));
                    if (iter != scope->end())
                        iter = scope->erase(iter);
                    scope->insert(iter, structure->convert(
                            rv.reg, structure->registers.get(rv.reg), rv.rtype));
                    scope->insert(iter, rv.rtype);
                } else {
                    rv.rtype = rv.utype = IR::Type::Bits::get(width, rv.reg->signed_); }
            } else {
                error("register %s width %d not supported for stateful_alu", rv.reg, rv.reg->width);
            }
        } else {
            error("%s reg property %s not a register", ext->name, rp);
        }
    } else {
        error("No reg property in %s", ext); }
    if (rv.reg) cache[rv.reg] = rv;
    return rv;
}

const IR::ActionProfile *P4V1::StatefulAluConverter::getSelectorProfile(
        P4V1::ProgramStructure *structure, const IR::Declaration_Instance *ext) {
    if (auto sel_bind = ext->properties.get<IR::Property>("selector_binding"_cs)) {
        auto ev = sel_bind->value->to<IR::ExpressionValue>();
        auto gref = ev ? ev->expression->to<IR::GlobalRef>() : nullptr;
        auto tbl = gref ? gref->obj->to<IR::V1Table>() : nullptr;
        if (!tbl) {
            error("%s is not a table", sel_bind);
            return nullptr; }
        auto ap = structure->action_profiles.get(tbl->action_profile);
        auto sel = ap ? structure->action_selectors.get(ap->selector) : nullptr;
        if (!sel) {
            error("No action selector for table %s", tbl);
            return nullptr; }
        return ap; }
    return nullptr;
}

const IR::Declaration_Instance *P4V1::StatefulAluConverter::convertExternInstance(
        P4V1::ProgramStructure *structure, const IR::Declaration_Instance *ext, cstring name,
        IR::IndexedVector<IR::Declaration> *scope) {
    auto *et = ext->type->to<IR::Type_Extern>();
    BUG_CHECK(et && et->name == "stateful_alu",
              "Extern %s is not stateful_alu type, but %s", ext, ext->type);

    auto *annots = new IR::Annotations();
    if (auto prop = ext->properties.get<IR::Property>("reduction_or_group"_cs)) {
        bool understood = false;
        if (auto ev = prop->value->to<IR::ExpressionValue>()) {
            if (auto pe = ev->expression->to<IR::PathExpression>()) {
                annots->addAnnotation("reduction_or_group"_cs,
                                      new IR::StringLiteral(pe->path->name));
                understood = true;
            }
        }
        ERROR_CHECK(understood, "%s: reduction_or_group provided on %s is not understood, because",
                    "it is not a PathExpression", ext->srcInfo, name);
    }

    if (auto ap = getSelectorProfile(structure, ext)) {
        LOG2("Creating apply function for SelectorAction " << ext->name);
        LOG6(ext);
        auto satype = new IR::Type_Name("SelectorAction");
        auto bit1 = IR::Type::Bits::get(1);
        auto *ctor_args = new IR::Vector<IR::Argument>({
                new IR::Argument(new IR::PathExpression(new IR::Path(
                    structure->action_profiles.get(ap)))) });
        auto *block = new IR::BlockStatement({
            CreateSaluApplyFunction::create(annots, structure, ext, bit1, bit1) });
        auto* externalName = new IR::StringLiteral(IR::ID("." + name));
        annots->addAnnotation(IR::ID("name"), externalName);
        // 1-bit selector alu may not need a 'fake' reg property
        // see sful_sel1.p4
        if (ext->properties.get<IR::Property>("reg"_cs)) {
            auto info = getRegInfo(structure, ext, structure->declarations);
            if (info.utype) {
                auto* regName = new IR::StringLiteral(IR::ID(info.reg->name));
                annots->addAnnotation(IR::ID("reg"), regName);
            }
        }
        auto *rv = new IR::Declaration_Instance(name, annots, satype, ctor_args, block);
        return rv->apply(TypeConverter(structure))->to<IR::Declaration_Instance>();
    }
    auto info = getRegInfo(structure, ext, structure->declarations);
    if (info.utype) {
        LOG2("Creating apply function for RegisterAction " << ext->name);
        LOG6(ext);
        auto reg_index_width = 32;
        // Frontend fix required before using correct index width
        // auto reg_index_width = info.reg->index_width();
        auto ratype = new IR::Type_Specialized(
            new IR::Type_Name("RegisterAction"),
            new IR::Vector<IR::Type>({info.rtype,
                IR::Type::Bits::get(reg_index_width), info.utype}));
        if (info.reg->instance_count == -1) {
            ratype = new IR::Type_Specialized(
                new IR::Type_Name("DirectRegisterAction"),
                new IR::Vector<IR::Type>({info.rtype, info.utype}));
        }

        auto *ctor_args = new IR::Vector<IR::Argument>({
                new IR::Argument(new IR::PathExpression(new IR::Path(info.reg->name))) });
        auto *math = CreateMathUnit::create(structure, ext, info.utype);
        if (math)
            scope->push_back(math);
        auto *block = new IR::BlockStatement({
            CreateSaluApplyFunction::create(annots, structure, ext, info.rtype, info.utype,
                                            math ? math->name.name : cstring(),
                                            info.reg->saturating) });
        auto* externalName = new IR::StringLiteral(IR::ID("." + name));
        annots->addAnnotation(IR::ID("name"), externalName);
        auto *rv = new IR::Declaration_Instance(name, annots, ratype, ctor_args, block);
        LOG3("Created apply function: " << *rv);
        return rv->apply(TypeConverter(structure))->to<IR::Declaration_Instance>();
    }

    BUG_CHECK(errorCount() > 0, "Failed to find utype for %s", ext);
    return ExternConverter::convertExternInstance(structure, ext, name, scope);
}

const IR::Statement *P4V1::StatefulAluConverter::convertExternCall(
            P4V1::ProgramStructure *structure, const IR::Declaration_Instance *ext,
            const IR::Primitive *prim) {
    auto *et = ext->type->to<IR::Type_Extern>();
    BUG_CHECK(et && et->name == "stateful_alu",
              "Extern %s is not stateful_alu type, but %s", ext, ext->type);
    const IR::Attached *target = getSelectorProfile(structure, ext);
    auto rtype = IR::Type::Bits::get(1);
    bool direct = false;
    auto reg_index_width = 32;
    if (!target) {
        auto info = getRegInfo(structure, ext, structure->declarations);
        rtype = info.utype;
        target = info.reg;
        BUG_CHECK(info.reg, "Extern %s has no associated register", et);
        direct = info.reg->instance_count < 0;
        // Frontend fix required before using correct index width
        // reg_index_width = info.reg->index_width();
    }
    if (!rtype) {
        BUG_CHECK(errorCount() > 0, "Failed to find rtype for %s", ext);
        return new IR::EmptyStatement(); }
    ExpressionConverter conv(structure);
    const IR::Statement *rv = nullptr;
    IR::BlockStatement *block = nullptr;
    auto extref = new IR::PathExpression(structure->externs.get(ext));
    auto method = new IR::Member(prim->srcInfo, extref, "execute"_cs);
    auto args = new IR::Vector<IR::Argument>();
    if (prim->name == "execute_stateful_alu") {
        BUG_CHECK(prim->operands.size() <= 2, "Wrong number of operands to %s", prim->name);
        if (prim->operands.size() == 2) {
            auto *idx = conv.convert(prim->operands.at(1));
            if (idx->is<IR::ListExpression>())
                error("%s%s expects a simple expression, not a %s",
                      prim->operands.at(1)->srcInfo, prim->name, idx);
            args->push_back(new IR::Argument(idx));
            if (direct)
                error("%scalling direct %s with an index", prim->srcInfo, target);
        } else if (!direct) {
            error("%scalling indirect %s with no index", prim->srcInfo, target); }
    } else if (prim->name == "execute_stateful_alu_from_hash") {
        BUG_CHECK(prim->operands.size() == 2, "Wrong number of operands to %s", prim->name);
        auto flc = structure->getFieldListCalculation(prim->operands.at(1));
        if (!flc) {
            error("%s: Expected a field_list_calculation", prim->operands.at(1));
            return nullptr; }
        block = new IR::BlockStatement;
        cstring temp = structure->makeUniqueName("temp"_cs);
        block = P4V1::generate_hash_block_statement(structure, prim, temp, conv, 2);
        args->push_back(new IR::Argument(new IR::Cast(IR::Type_Bits::get(reg_index_width),
                        new IR::PathExpression(new IR::Path(temp)))));
    } else if (prim->name == "execute_stateful_log") {
        BUG_CHECK(prim->operands.size() == 1, "Wrong number of operands to %s", prim->name);
        method = new IR::Member(prim->srcInfo, extref, "execute_log");
    } else {
        BUG("Unknown method %s in stateful_alu", prim->name); }

    auto mc = new IR::MethodCallExpression(prim->srcInfo, rtype, method, args);
    if (auto prop = ext->properties.get<IR::Property>("output_dst"_cs)) {
        if (auto ev = prop->value->to<IR::ExpressionValue>()) {
            auto type = ev->expression->type;
            if (ext->properties.get<IR::Property>("reduction_or_group"_cs)) {
                const IR::Expression *expr = mc;
                if (expr->type != type)
                    expr = new IR::Cast(type, expr);
                expr = new IR::BOr(conv.convert(ev->expression), expr);
                rv = structure->assign(prim->srcInfo, conv.convert(ev->expression), expr, type);
            } else {
                rv = structure->assign(prim->srcInfo, conv.convert(ev->expression), mc, type);
            }
        } else {
            error("%s: output_dst property is not an expression", prop->value->srcInfo);
            return nullptr; }
    } else {
        rv = new IR::MethodCallStatement(prim->srcInfo, mc); }
    if (block) {
        block->push_back(rv);
        rv = block; }
    return rv;
}

P4V1::StatefulAluConverter P4V1::StatefulAluConverter::singleton;
