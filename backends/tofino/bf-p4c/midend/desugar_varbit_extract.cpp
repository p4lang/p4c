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

#include "desugar_varbit_extract.h"

#include <lib/bitvec.h>

#include "bf-p4c/bf-p4c-options.h"
#include "bf-p4c/common/utils.h"
#include "bf-p4c/device.h"
#include "frontends/common/constantFolding.h"

namespace BFN {

/// Does the header type contain any varbit field?
static bool has_varbit(const IR::Type_Header *hdr) {
    CHECK_NULL(hdr);
    return std::any_of(hdr->fields.begin(), hdr->fields.end(), [](const IR::StructField *fld) {
        return fld->type->is<IR::Type_Varbits>();
    });
}

/**
 * \ingroup OptimizeAndCheckVarbitAccess
 */
struct ErrorOnUnsupportedVarbitUse : public Inspector {
    // We limit the use of varbit field to parser and deparser, which is
    // sufficient for skipping through header options.
    // To support the use of varbit field in the MAU add a few twists to
    // the problem (TODO).

    bool preorder(const IR::Expression *expr) override {
        if (expr->type->is<IR::Type_Varbits>()) {
            auto control = findContext<IR::BFN::TnaControl>();
            if (control) {
                fatal_error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
                            "%1%: use of varbit field is only supported in parser and deparser "
                            "currently",
                            expr);
            }
        }

        return true;
    }

    // TODO: When we enable assignment of varbits, we need to make sure the dead emit elimination
    // works correctly even with assignment.
    bool preorder(const IR::MethodCallExpression *mc) override {
        if (auto mem = mc->method->to<IR::Member>();
            mem && mem->member == IR::Type_Header::setValid) {
            auto hdr = mem->expr->type->to<IR::Type_Header>();
            BUG_CHECK(hdr, "setValid is called on non-header type %1%", mem->expr);

            if (has_varbit(hdr)) {
                fatal_error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
                            "%1%: cannot set header that contains a varbit field as valid. The "
                            "compiler currently does not support assigning values to varbit "
                            "fields. Therefore, if a header containing a varbit field is set as "
                            "valid rather than parsed, the value of the varbit field will always "
                            "be undefined.\n"
                            "Suggestion: if you need to set only non-varbit parts of the header, "
                            "please split the header.",
                            mc);
            }
        }
        return true;
    }

    // TODO: When we enable assignment of varbits, we need to make sure the dead emit elimination
    // works correctly even with assignment.
    bool preorder(const IR::AssignmentStatement *asgn) override {
        if (asgn->right->type->is<IR::Type_Varbits>()) {
            fatal_error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
                        "%1%: cannot assign varbit field. The compiler currently does not "
                        "support assigning values to varbit fields.",
                        asgn->right);
        }
        if (auto hdr = asgn->right->type->to<IR::Type_Header>(); hdr && has_varbit(hdr)) {
            fatal_error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
                        "%1%: cannot assign header that contains a varbit field. The "
                        "compiler currently does not support assigning values to varbit "
                        "fields. Therefore, a header that contains varbit cannot be assigned.\n"
                        "Suggestion: if you need to set only non-varbit parts of the header, "
                        "please split the header.",
                        asgn->right);
        }
        return true;
    }
};

struct CollectVariables : public Inspector {
    bool preorder(const IR::Expression *expr) override {
        if (expr->is<IR::Member>() || expr->is<IR::Slice>() || expr->is<IR::TempVar>() ||
            expr->is<IR::PathExpression>()) {
            rv.push_back(expr);
            return false;
        }
        return true;
    }

    std::vector<const IR::Expression *> rv;

    CollectVariables() {}
};

struct EvaluateForVar : public PassManager {
    struct SubstituteVar : public Transform {
        const IR::Expression *var;
        unsigned val;

        IR::Node *preorder(IR::Expression *expr) override {
            if (expr->equiv(*var)) {
                auto width = var->type->width_bits();
                auto constant = new IR::Constant(IR::Type::Bits::get(width), val);
                return constant;
            } else {
                return expr;
            }
        }

        SubstituteVar(const IR::Expression *var, unsigned val) : var(var), val(val) {}
    };

    EvaluateForVar(P4::ReferenceMap *rm, P4::TypeMap *tm, const IR::Expression *var, unsigned val)
        : refMap(*rm), typeMap(*tm) {
        addPasses({new SubstituteVar(var, val), new P4::ConstantFolding(&typeMap, false)});
    }

    // ConstantFolding has side effects on typeMap, make these local copies
    // so that we don't modify the original map
    P4::ReferenceMap refMap;
    P4::TypeMap typeMap;
};

/**
 * \ingroup DesugarVarbitExtract
 */
class CollectVarbitExtract : public Inspector {
    P4::ReferenceMap *refMap;
    P4::TypeMap *typeMap;

 public:
    std::map<const IR::ParserState *, const IR::BFN::TnaParser *> state_to_parser;

    std::map<const IR::ParserState *, const IR::Type_Header *> state_to_varbit_header;
    // Map to store if the varbit extract length is dynamic
    std::map<cstring, ordered_set<const IR::ParserState *>> varbit_hdr_instance_to_variable_state;

    std::map<const IR::ParserState *, const IR::Expression *> state_to_encode_var;

    std::map<const IR::ParserState *, const IR::AssignmentStatement *> state_to_csum_verify;
    // Map to store if the varbit extract length is constant
    std::map<cstring, std::map<unsigned, ordered_set<const IR::ParserState *>>>
        varbit_hdr_instance_to_constant_state;
    std::map<cstring, const IR::StructField *> varbit_hdr_instance_to_varbit_field;

    std::map<const IR::ParserState *, std::set<const IR::Expression *>> state_to_verify_exprs;

    std::map<const IR::ParserState *, cstring> state_to_header_instance;

    std::map<const IR::ParserState *, std::set<const IR::MethodCallExpression *>> state_to_csum_add;

    std::map<const IR::ParserState *, std::map<unsigned, unsigned>> state_to_match_to_length;

    // reverse map of above
    std::map<const IR::ParserState *, std::map<unsigned, std::set<unsigned>>>
        state_to_length_to_match;

    std::map<const IR::ParserState *, std::set<unsigned>> state_to_reject_matches;

    std::map<const IR::Type_Header *, const IR::StructField *> header_type_to_varbit_field;
    std::map<const IR::StructField *, const IR::Expression *> varbit_field_to_extract_call_path;

 private:
    bool is_legal_runtime_value(const IR::Expression *verify, const IR::Expression *encode_var,
                                unsigned val);

    bool is_legal_runtime_value(const IR::ParserState *state, const IR::Expression *encode_var,
                                unsigned val);

    const IR::Constant *evaluate(const IR::Expression *varsize_expr,
                                 const IR::Expression *encode_var, unsigned val);

    bool enumerate_varbit_field_values(const IR::MethodCallExpression *call,
                                       const IR::ParserState *state,
                                       const IR::StructField *varbit_field,
                                       const IR::Expression *varsize_expr,
                                       const IR::Expression *&encode_var,
                                       std::map<unsigned, unsigned> &match_to_length,
                                       std::map<unsigned, std::set<unsigned>> &length_to_match,
                                       std::set<unsigned> &reject_matches, cstring header_name);

    void enumerate_varbit_field_values(const IR::MethodCallExpression *call,
                                       const IR::ParserState *state,
                                       const IR::Expression *varsize_expr,
                                       const IR::Type_Header *hdr_type, cstring headerName);

    bool preorder(const IR::MethodCallExpression *) override;
    bool preorder(const IR::AssignmentStatement *) override;

 public:
    CollectVarbitExtract(P4::ReferenceMap *refMap, P4::TypeMap *typeMap)
        : refMap(refMap), typeMap(typeMap) {}
};

const IR::Constant *CollectVarbitExtract::evaluate(const IR::Expression *varsize_expr,
                                                   const IR::Expression *encode_var, unsigned val) {
    auto clone = varsize_expr->clone();

    EvaluateForVar evaluator(refMap, typeMap, encode_var, val);
    auto rv = clone->apply(evaluator);

    auto constant = rv->to<IR::Constant>();
    BUG_CHECK(constant, "varsize expr did not evaluate to constant?");

    return constant;
}

bool CollectVarbitExtract::is_legal_runtime_value(const IR::Expression *verify,
                                                  const IR::Expression *encode_var, unsigned val) {
    auto clone = verify->clone();

    EvaluateForVar evaluator(refMap, typeMap, encode_var, val);
    auto rv = clone->apply(evaluator);

    if (auto bl = rv->to<IR::BoolLiteral>()) {
        if (bl->value == 0) return false;
    }

    return true;
}

bool CollectVarbitExtract::is_legal_runtime_value(const IR::ParserState *state,
                                                  const IR::Expression *encode_var, unsigned val) {
    if (state_to_verify_exprs.count(state)) {
        for (auto verify : state_to_verify_exprs.at(state)) {
            if (!is_legal_runtime_value(verify, encode_var, val)) return false;
        }
    }

    return true;
}

void check_compile_time_constant(const IR::Constant *c, const IR::MethodCallExpression *call,
                                 int varbit_field_size, bool zero_ok = false) {
    if (c->asInt() < 0 || (!zero_ok && c->asInt() == 0)) {
        cstring hint;

        if (BackendOptions().langVersion == CompilerOptions::FrontendVersion::P4_16)
            hint = "Please make sure the encoding variable is cast to bit<32>."_cs;

        ::fatal_error("Varbit field size expression evaluates to invalid value %1%: %2% \n%3%",
                      c->asInt(), call, hint);
    }

    if (c->asInt() % 8) {
        ::fatal_error("Varbit field size expression evaluates to non byte-aligned value %1%: %2%",
                      c->asInt(), call);
    }

    if (c->asInt() > varbit_field_size) {
        ::fatal_error("%1% exceeds varbit field size %2%: %3%", c->asInt(), varbit_field_size,
                      call);
    }
}

bool CollectVarbitExtract::enumerate_varbit_field_values(
    const IR::MethodCallExpression *call, const IR::ParserState *state,
    const IR::StructField *varbit_field, const IR::Expression *varsize_expr,
    const IR::Expression *&encode_var, std::map<unsigned, unsigned> &match_to_length,
    std::map<unsigned, std::set<unsigned>> &length_to_match, std::set<unsigned> &reject_matches,
    cstring header_name) {
    CollectVariables find_encode_var;
    varsize_expr->apply(find_encode_var);

    if (find_encode_var.rv.size() == 0)
        ::fatal_error("No varbit length encoding variable in %1%.", call);
    else if (find_encode_var.rv.size() > 1)
        ::fatal_error("Varbit expression %1% contains more than one variable.", call);

    encode_var = find_encode_var.rv[0];

    unsigned long var_bitwidth = encode_var->type->width_bits();

    if (var_bitwidth > 32) {  // already checked in frontend?
        ::fatal_error("Varbit length encoding variable requires more than 32 bits: %1%",
                      encode_var);
    }

    unsigned branches_needed = 0;
    bool too_many_branches = false;

    for (unsigned long i = 0; i < (1UL << var_bitwidth); i++) {
        if (is_legal_runtime_value(state, encode_var, i)) {
            auto c = evaluate(varsize_expr, encode_var, i);

            auto varbit_field_size = varbit_field->type->to<IR::Type_Varbits>()->size;
            if (!c->fitsInt() || c->asInt() > varbit_field_size) {
                reject_matches.insert(i);
                LOG4("compile time constant exceeds varbit field size: " << c->asUnsigned());
            } else {
                check_compile_time_constant(c, call, varbit_field_size, true);
                match_to_length[i] = c->asUnsigned();
                length_to_match[c->asUnsigned()].insert(i);
            }
        } else {
            reject_matches.insert(i);
        }

        branches_needed++;

        if (branches_needed > 1024) {
            too_many_branches = true;
            break;
        }
    }

    if (too_many_branches) {
        ::fatal_error(
            "Varbit extract requires too many parser branches to implement. "
            "Consider rewriting the variable length expression or shrink the varbit size "
            "to reduce the number of possible runtime values: %1%",
            call);
        return false;
    }

    LOG2(varsize_expr << " needs " << branches_needed << " branches to implement");
    LOG2(varsize_expr << " evaluate to compile time constants:");

    for (auto &kv : match_to_length) LOG2(encode_var << " = " << kv.first << " : " << kv.second);

    for (auto &v : reject_matches) LOG2(encode_var << " = " << v << " : reject");
    varbit_hdr_instance_to_variable_state[header_name].insert(state);
    return true;
}

void CollectVarbitExtract::enumerate_varbit_field_values(const IR::MethodCallExpression *call,
                                                         const IR::ParserState *state,
                                                         const IR::Expression *varsize_expr,
                                                         const IR::Type_Header *hdr_type,
                                                         cstring headerName) {
    if (state_to_varbit_header.count(state)) {
        fatal_error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
                    "%1%: multiple varbit fields in a parser state are currently unsupported",
                    call);
    }

    const IR::StructField *varbit_field = nullptr;

    for (auto field : hdr_type->fields) {
        if (field->type->is<IR::Type::Varbits>()) {
            varbit_field = field;
            break;
        }
    }

    if (!varbit_field) return;
    varbit_hdr_instance_to_varbit_field[headerName] = varbit_field;
    const IR::Expression *encode_var = nullptr;
    std::map<unsigned, unsigned> match_to_length;
    std::map<unsigned, std::set<unsigned>> length_to_match;
    std::set<unsigned> reject_matches;

    if (auto c = varsize_expr->to<IR::Constant>()) {
        auto varbit_field_size = varbit_field->type->to<IR::Type_Varbits>()->size;

        check_compile_time_constant(c, call, varbit_field_size);
        match_to_length[0] = c->asUnsigned();  // use 0 as key, but really don't care
        length_to_match[c->asUnsigned()].insert(0);
        varbit_hdr_instance_to_constant_state[headerName][c->asUnsigned()].insert(state);
    } else {
        bool ok = enumerate_varbit_field_values(call, state, varbit_field, varsize_expr, encode_var,
                                                match_to_length, length_to_match, reject_matches,
                                                headerName);
        if (!ok) return;
    }

    state_to_varbit_header[state] = hdr_type;
    state_to_encode_var[state] = encode_var;
    state_to_match_to_length[state] = match_to_length;
    state_to_length_to_match[state] = length_to_match;
    state_to_reject_matches[state] = reject_matches;

    auto expr = (*call->arguments)[0]->expression;
    const IR::Expression *path = nullptr;
    if (auto member = expr->to<IR::Member>()) {
        path = member->expr;
    } else if (expr->is<IR::PathExpression>()) {
        path = expr;
    } else {
        BUG("Unknown extract call path %1%", expr);
    }

    varbit_field_to_extract_call_path[varbit_field] = path;

    state_to_header_instance[state] = headerName;
    header_type_to_varbit_field[hdr_type] = varbit_field;
}

const IR::StructField *get_varbit_structfield(const IR::Member *member) {
    auto header = member->expr->to<IR::Member>();
    auto headerType = header->type->to<IR::Type_Header>();
    for (auto field : headerType->fields) {
        if (field->name == member->member) {
            return field;
        }
    }
    BUG("No varbit found in %1%", header);
    return nullptr;
}

bool CollectVarbitExtract::preorder(const IR::AssignmentStatement *astmt) {
    if (auto state = findContext<IR::ParserState>()) {
        bool seen_varbit = state_to_varbit_header.count(state);
        if (!seen_varbit) return false;
        auto right = astmt->right;
        if (right->is<IR::BFN::ReinterpretCast>())
            right = right->to<IR::BFN::ReinterpretCast>()->expr;

        if (auto mc = right->to<IR::MethodCallExpression>()) {
            if (auto method = mc->method->to<IR::Member>()) {
                if (auto path = method->expr->to<IR::PathExpression>()) {
                    auto type = path->type->to<IR::Type_Extern>();
                    if (!type) return false;
                    if (type->name != "Checksum") return false;
                    if (method->member == "verify") state_to_csum_verify[state] = astmt;
                }
            }
        }
    }
    return false;
}

bool CollectVarbitExtract::preorder(const IR::MethodCallExpression *call) {
    if (auto state = findContext<IR::ParserState>()) {
        bool seen_varbit = state_to_varbit_header.count(state);

        if (auto method = call->method->to<IR::Member>()) {
            if (method->member == "extract") {
                auto expr = (*call->arguments)[0]->expression;

                if (call->arguments->size() == 2) {
                    auto varsize_expr = (*call->arguments)[1]->expression;

                    LOG3("found varbit in " << state->name);
                    LOG3("varsize expr is " << varsize_expr);

                    if (seen_varbit) {
                        fatal_error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
                                    "%1%: multiple varbit fields in a parser state are currently "
                                    "unsupported",
                                    call);
                    }
                    const IR::Type_Header *hdr_type = nullptr;
                    cstring headerName;

                    if (auto header = expr->to<IR::Member>()) {
                        hdr_type = header->type->to<IR::Type_Header>();
                        headerName = header->member;
                    } else if (auto headerPath = expr->to<IR::PathExpression>()) {
                        hdr_type = headerPath->type->to<IR::Type_Header>();
                        auto path = headerPath->path->to<IR::Path>();
                        headerName = path->name;
                    } else {
                        error("Unsupported header type %1%", expr);
                    }
                    enumerate_varbit_field_values(call, state, varsize_expr, hdr_type, headerName);

                    auto parser = findContext<IR::BFN::TnaParser>();
                    state_to_parser[state] = parser;
                    seen_varbit = true;
                } else if (seen_varbit) {
                    fatal_error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
                                "%1%: current compiler implementation requires the "
                                "varbit header to be extracted last in the parser state",
                                call);
                }
            } else if (method->member == "add" || method->member == "subtract") {
                auto path = method->expr->to<IR::PathExpression>();
                if (!path) return false;
                auto type = path->type->to<IR::Type_Extern>();
                if (!type) return false;
                if (type->name != "Checksum") return false;

                if (auto add_expr = (*call->arguments)[0]->expression->to<IR::StructExpression>()) {
                    for (auto field : add_expr->components) {
                        if (auto member = field->expression->to<IR::Member>()) {
                            if (member->type->is<IR::Type_Varbits>()) {
                                if (method->member == "subtract") {
                                    fatal_error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
                                                "Checksum subtract is currently not supported to "
                                                "have varbit fields %1%",
                                                call);
                                }
                                state_to_csum_add[state].insert(call);
                            }
                        }
                    }
                }
            }
        } else if (auto path = call->method->to<IR::PathExpression>()) {
            if (path->path->name == "verify") {
                warning("Parser \"verify\" is currently unsupported %s", call->srcInfo);
            }
        }
    }
    return false;
}

static cstring create_instance_name(cstring name) {
    std::string str(name.c_str());
    str = str.substr(0, str.length() - 2);
    return cstring(str.c_str());
}

/**
 * \ingroup DesugarVarbitExtract
 */
class RewriteVarbitUses : public Modifier {
    const CollectVarbitExtract &cve;
    int tcam_row_usage_estimation = 0;

    std::map<const IR::ParserState *, ordered_map<unsigned, const IR::ParserState *>>
        state_to_branch_states;

    std::map<const IR::ParserState *, const IR::ParserState *> state_to_end_state;

 public:
    // To get all the headers associated with length L, find all the header types with
    // length <= L
    ordered_map<cstring, ordered_map<unsigned, IR::Type_Header *>>
        varbit_hdr_instance_to_header_types;

    ordered_map<std::pair<cstring, unsigned>, ordered_map<unsigned, IR::Type_Header *>>
        varbit_hdr_instance_to_header_types_by_base;

    std::map<const IR::StructField *, IR::Type_Header *> varbit_field_to_post_header_type;

    std::map<cstring, IR::Vector<IR::StructField>> tuple_types_to_rewrite;

 private:
    profile_t init_apply(const IR::Node *root) override;

    const IR::ParserState *create_branch_state(const IR::BFN::TnaParser *parser,
                                               const IR::ParserState *state,
                                               const IR::Expression *select,
                                               const IR::StructField *varbit_field, unsigned length,
                                               unsigned base_length, cstring name);

    void create_varbit_header_type(const IR::Type_Header *orig_hdr, cstring orig_hdr_inst,
                                   const IR::StructField *varbit_field, unsigned current_hdr_size,
                                   unsigned varbit_field_size, unsigned base_size);

    void create_branches(const IR::ParserState *state, const IR::StructField *varbit_field,
                         unsigned prev_length);

    void find_and_replace_setinvalid(const IR::AssignmentStatement *assign,
                                     IR::IndexedVector<IR::StatOrDecl> &comp);
    const IR::ParserState *create_end_state(const IR::BFN::TnaParser *parser,
                                            const IR::ParserState *state, cstring name,
                                            const IR::StructField *varbit_field,
                                            const IR::Type_Header *orig_header,
                                            cstring orig_hdr_name);

    bool preorder(IR::BFN::TnaParser *) override;
    void postorder(IR::BFN::TnaParser *) override;
    bool preorder(IR::ParserState *) override;

    bool preorder(IR::MethodCallExpression *) override;
    bool preorder(IR::BlockStatement *) override;

    IR::IndexedVector<IR::NamedExpression> filter_post_header_fields(
        const IR::IndexedVector<IR::NamedExpression> &components);

    bool preorder(IR::StructExpression *list) override;
    bool preorder(IR::Member *member) override;

 public:
    explicit RewriteVarbitUses(const CollectVarbitExtract &cve) : cve(cve) {}
};

Modifier::profile_t RewriteVarbitUses::init_apply(const IR::Node *root) {
    for (const auto &[name, states] : cve.varbit_hdr_instance_to_constant_state) {
        // If the length is declared as a constant, the creation of branches
        // for each state should happen in the ascending order of the lengths
        unsigned prev_length = 0;
        auto varbit_field = cve.varbit_hdr_instance_to_varbit_field.at(name);
        for (const auto &len_states : states) {
            for (const auto *state : len_states.second) {
                create_branches(state, varbit_field, prev_length);
                prev_length = len_states.first;
            }
        }
    }
    for (const auto &[name, states] : cve.varbit_hdr_instance_to_variable_state) {
        for (const auto &state : states) {
            auto varbit_field = cve.varbit_hdr_instance_to_varbit_field.at(name);
            create_branches(state, varbit_field, 0);
        }
    }
    return Modifier::init_apply(root);
}

static IR::MethodCallStatement *create_extract_statement(const IR::BFN::TnaParser *parser,
                                                         const IR::Expression *path,
                                                         const IR::Type *type, cstring header) {
    auto packetInParam = parser->tnaParams.at("pkt"_cs);
    auto method = new IR::Member(new IR::PathExpression(packetInParam), IR::ID("extract"));
    auto typeArgs = new IR::Vector<IR::Type>({type});
    auto args =
        new IR::Vector<IR::Argument>({new IR::Argument(new IR::Member(type, path, header))});
    auto *callExpr = new IR::MethodCallExpression(method, typeArgs, args);
    return new IR::MethodCallStatement(callExpr);
}

static IR::MethodCallStatement *create_add_statement(const IR::Member *method,
                                                     const IR::Expression *path,
                                                     const IR::Type_Header *header) {
    auto listVec = IR::Vector<IR::Expression>();
    auto headerName = create_instance_name(header->name);
    for (const auto *f : header->fields) {
        listVec.push_back(new IR::Member(f->type, new IR::Member(path, headerName), f->name));
    }
    auto args = new IR::Vector<IR::Argument>({new IR::Argument(new IR::ListExpression(listVec))});
    auto typeArgs = new IR::Vector<IR::Type>({args->at(0)->expression->type});
    auto addCall = new IR::MethodCallExpression(method, typeArgs, args);
    return new IR::MethodCallStatement(addCall);
}
const IR::ParserState *RewriteVarbitUses::create_branch_state(
    const IR::BFN::TnaParser *parser, const IR::ParserState *state, const IR::Expression *select,
    const IR::StructField *varbit_field, unsigned length, unsigned base_length, cstring name) {
    for (const auto &kv : state_to_branch_states) {
        auto p = cve.state_to_parser.at(kv.first);
        if (p == parser) {
            for (const auto &s : kv.second) {
                if (name == s.second->name) return s.second;
            }
        }
    }

    IR::IndexedVector<IR::StatOrDecl> statements;
    auto hdr_instance = cve.state_to_header_instance.at(state);
    auto path = cve.varbit_field_to_extract_call_path.at(varbit_field);
    // Extract all the headers in map varbit_hdr_instance_to_header_types_by_base until
    // key is greater than length
    for (auto &header_to_length :
         varbit_hdr_instance_to_header_types_by_base[std::make_pair(hdr_instance, base_length)]) {
        auto header_length = header_to_length.first;
        if (header_length > length) break;

        auto header = header_to_length.second;
        auto varbit_hdr_inst = create_instance_name(header->name);
        auto extract = create_extract_statement(parser, path, header, varbit_hdr_inst);
        statements.push_back(extract);
        LOG3("Added extract statement for header " << header->name << " in state " << name);
        if (cve.state_to_csum_add.count(state)) {
            for (auto call : cve.state_to_csum_add.at(state)) {
                auto method = call->method->to<IR::Member>();
                statements.push_back(create_add_statement(method, path, header));
            }
        }
    }
    auto branch_state = new IR::ParserState(name, statements, select);

    LOG3("create state " << name);

    return branch_state;
}

const IR::ParserState *RewriteVarbitUses::create_end_state(const IR::BFN::TnaParser *parser,
                                                           const IR::ParserState *state,
                                                           cstring name,
                                                           const IR::StructField *varbit_field,
                                                           const IR::Type_Header *orig_header,
                                                           cstring orig_hdr_name) {
    for (const auto &kv : state_to_branch_states) {
        const auto p = cve.state_to_parser.at(kv.first);
        if (p == parser) {
            for (const auto &s : kv.second) {
                if (name == s.second->name) return s.second;
            }
        }
    }

    for (const auto &kv : state_to_end_state) {
        const auto p = cve.state_to_parser.at(kv.first);
        if (p == parser) {
            if (name == kv.second->name) return kv.second;
        }
    }

    // If varbit field is in the middle of the header type, we need to create
    // a seperate header type for the fields that follow the varbit field.
    //
    // header xxx_t {
    //   bit<4>   a;
    //   bit<4>   b;
    //   varbit<320> c;
    //   bit<32>  x;
    //   bit<16>  y;
    // }

    cstring post_hdr_name = orig_hdr_name + "_"_cs + varbit_field->name + "_post_t"_cs;
    auto post_hdr = new IR::Type_Header(post_hdr_name);

    bool seen_varbit = false;
    for (auto field : orig_header->fields) {
        if (seen_varbit) post_hdr->fields.push_back(field);

        if (field->type->is<IR::Type_Varbits>()) seen_varbit = true;
    }

    IR::IndexedVector<IR::StatOrDecl> statements;

    auto path = cve.varbit_field_to_extract_call_path.at(varbit_field);

    if (post_hdr->fields.size()) {
        varbit_field_to_post_header_type[varbit_field] = post_hdr;

        auto post_hdr_inst = create_instance_name(post_hdr_name);
        auto extract = create_extract_statement(parser, path, post_hdr, post_hdr_inst);

        statements.push_back(extract);
    }

    if (cve.state_to_csum_add.count(state)) {
        for (auto call : cve.state_to_csum_add.at(state)) {
            auto method = call->method->to<IR::Member>();
            statements.push_back(create_add_statement(method, path, post_hdr));
        }
    }

    if (cve.state_to_csum_verify.count(state))
        statements.push_back(cve.state_to_csum_verify.at(state));

    return new IR::ParserState(name, statements, state->selectExpression);
}

void RewriteVarbitUses::create_varbit_header_type(const IR::Type_Header *orig_hdr,
                                                  cstring orig_hdr_inst,
                                                  const IR::StructField *varbit_field,
                                                  unsigned current_hdr_size,
                                                  unsigned varbit_field_size, unsigned base_size) {
    auto hdr_base = std::make_pair(orig_hdr_inst, base_size);
    // Verify that if the header already exists, it's already been inserted in this base size
    if (varbit_hdr_instance_to_header_types.count(orig_hdr_inst) &&
        varbit_hdr_instance_to_header_types.at(orig_hdr_inst).count(varbit_field_size)) {
        BUG_CHECK(
            varbit_hdr_instance_to_header_types_by_base.count(hdr_base) &&
                varbit_hdr_instance_to_header_types_by_base.at(hdr_base).count(varbit_field_size),
            "Already created %1%b slice of %2% but it is missing from the %3%b base",
            current_hdr_size, orig_hdr->name, base_size);
    }

    if (!varbit_hdr_instance_to_header_types_by_base.count(hdr_base) ||
        !varbit_hdr_instance_to_header_types_by_base.at(hdr_base).count(varbit_field_size)) {
        cstring name = orig_hdr->name + "_"_cs + orig_hdr_inst + "_"_cs + varbit_field->name +
                       "_"_cs + cstring::to_cstring(varbit_field_size) + "b_t";
        auto hdr = new IR::Type_Header(name);
        auto field_type = IR::Type::Bits::get(current_hdr_size);
        auto field = new IR::StructField("field", field_type);
        hdr->fields.push_back(field);
        varbit_hdr_instance_to_header_types[orig_hdr_inst][varbit_field_size] = hdr;
        varbit_hdr_instance_to_header_types_by_base[hdr_base][varbit_field_size] = hdr;
    }
}

void RewriteVarbitUses::create_branches(const IR::ParserState *state,
                                        const IR::StructField *varbit_field, unsigned prev_length) {
    ordered_map<unsigned, const IR::ParserState *> length_to_branch_state;

    auto parser = cve.state_to_parser.at(state);
    auto orig_hdr = cve.state_to_varbit_header.at(state);
    auto orig_hdr_inst = cve.state_to_header_instance.at(state);
    auto encode_var = cve.state_to_encode_var.at(state);

    auto &value_map = cve.state_to_length_to_match.at(state);

    cstring no_option_state_name =
        "parse_"_cs + orig_hdr_inst + "_"_cs + varbit_field->name + "_end"_cs;
    const IR::ParserState *no_option_state = nullptr;
    unsigned base_length = value_map.begin()->first;
    for (auto &kv : value_map) {
        auto length = kv.first;

        if (length) {
            if (length - prev_length != 0) {
                create_varbit_header_type(orig_hdr, orig_hdr_inst, varbit_field,
                                          length - prev_length, length, base_length);
                prev_length = length;
            }

            const IR::Expression *select = nullptr;

            if (encode_var)
                select = new IR::PathExpression(no_option_state_name);
            else  // unconditional transition (varsize expr is constant)
                select = state->selectExpression;

            cstring name = "parse_"_cs + orig_hdr_inst + "_"_cs + varbit_field->name + "_"_cs +
                           cstring::to_cstring(length) + "b";

            auto branch_state =
                create_branch_state(parser, state, select, varbit_field, length, base_length, name);

            length_to_branch_state[length] = branch_state;
        } else {
            no_option_state = create_end_state(parser, state, no_option_state_name, varbit_field,
                                               orig_hdr, orig_hdr_inst);

            length_to_branch_state[length] = no_option_state;
        }
    }

    if (!no_option_state) {
        no_option_state = create_end_state(parser, state, no_option_state_name, varbit_field,
                                           orig_hdr, orig_hdr_inst);
    }

    state_to_branch_states[state] = length_to_branch_state;
    state_to_end_state[state] = no_option_state;
}

bool RewriteVarbitUses::preorder(IR::BFN::TnaParser *parser) {
    tcam_row_usage_estimation = 0;
    auto orig = getOriginal<IR::P4Parser>();

    bool has_varbit = false;

    for (auto &kv : cve.state_to_parser) {
        if (kv.second == orig) {
            has_varbit = true;
            break;
        }
    }

    if (!has_varbit) return true;

    std::set<const IR::ParserState *> states;

    for (auto &kv : state_to_branch_states) {
        if (cve.state_to_parser.at(kv.first) == orig) {
            for (auto &ms : kv.second) states.insert(ms.second);
        }
    }

    for (auto &kv : state_to_end_state) {
        if (cve.state_to_parser.at(kv.first) == orig) {
            states.insert(kv.second);
        }
    }

    for (auto s : states) parser->states.push_back(s);

    return true;
}

void RewriteVarbitUses::postorder(IR::BFN::TnaParser *p) {
    LOG5("Total expected tcam usage by varbit extracts: " << tcam_row_usage_estimation);
    // Desugaring varbit extracts has potential to inflate the IR significantly, making
    // the rest of the compilation very slow. For user's convenience, we try to estimate
    // the TCAM rows usage by counting all parser select branches we generate for varbit
    // extracts. If this estimation is already much larger than TCAM rows count available
    // on the device we bail out early.
    if (tcam_row_usage_estimation > Device::pardeSpec().numTcamRows() * 2) {
        ::fatal_error(
            "Varbit extracts require too many parser branches to implement. "
            "Parser %1% needs %2% transitions for varbit extracts, which is more than "
            "%3% TCAM rows available. "
            "Consider rewriting the variable length expression or shrink the varbit "
            "sizes to reduce the total number of possible runtime values.",
            p->name, tcam_row_usage_estimation, Device::pardeSpec().numTcamRows());
    }
}

static IR::SelectCase *create_select_case(unsigned bitwidth, unsigned value, unsigned mask,
                                          cstring next_state) {
    auto type = IR::Type::Bits::get(bitwidth);
    auto value_expr = new IR::Constant(type, value);
    auto mask_expr = new IR::Constant(type, mask);
    auto next_state_expr = new IR::PathExpression(next_state);
    return new IR::SelectCase(new IR::Mask(value_expr, mask_expr), next_state_expr);
}

struct Match {
    unsigned value = 0;
    unsigned mask = 0;
};

/// Try to be smart about multiple matches - if we can collapse consecutive values from one value
/// set into a ternary match that matches them exactly, we do so and proceed to match the remaining
/// values. This is not optimal, sometimes grouping non-consective values would provide lower
/// number of matches, but this is a sufficiently simple heuristic.
static std::vector<Match> merge_matches(const std::set<unsigned> &values0, unsigned mask) {
    if (values0.empty()) return {};
    // avoid having to keep counts for each range, use random access iterator difference instead
    std::vector<unsigned> values(values0.begin(), values0.end());
    std::vector<Match> out;
    LOG9("Trying to cover " << values.size() << " values between " << values.front() << " and "
                            << values.back());

    unsigned irrelevant_bits_candidate = 0;
    Match match_candidate;
    auto low = values.begin();
    auto high = values.begin();
    auto last_high = low;
    while (high != values.end()) {
        LOG9("  adding " << *high << " to match set");
        for (auto it = low; it != high; ++it) {
            irrelevant_bits_candidate |= *high ^ *it;
        }
        unsigned match_size = 1u << bv::popcount(irrelevant_bits_candidate);
        if (match_size == high - low + 1) {
            match_candidate.value = *low;
            match_candidate.mask = mask & ~irrelevant_bits_candidate;
            last_high = high;
        }
        // if we oversoot or if we got to the end of values, we save the last working match and
        // continue after its end
        if (match_size > (values.end() - low + 1) || high == std::prev(values.end())) {
            LOG9("  covered " << (last_high - low + 1) << " values between " << *low << " and "
                              << *last_high << " with " << match_candidate.value << " &&& "
                              << match_candidate.mask);
            out.push_back(match_candidate);
            low = high = std::next(last_high);
            match_candidate = Match();
            irrelevant_bits_candidate = 0;
            continue;  // skip high increment
        }
        ++high;
    }

    return out;
}

bool RewriteVarbitUses::preorder(IR::ParserState *state) {
    auto orig = getOriginal<IR::ParserState>();
    IR::IndexedVector<IR::StatOrDecl> components;
    for (auto a : orig->components) {
        // Remove verify call from original state
        if (cve.state_to_csum_verify.count(orig) && cve.state_to_csum_verify.at(orig)->equiv(*a)) {
            continue;
            // Replace varbit header setInvalid
        } else if (auto assign = a->to<IR::AssignmentStatement>()) {
            find_and_replace_setinvalid(assign, components);
        }
        components.push_back(a);
    }
    state->components = components;

    if (!state_to_branch_states.count(orig)) return true;

    auto &length_to_branch_state = state_to_branch_states.at(orig);
    auto &length_to_match = cve.state_to_length_to_match.at(orig);

    IR::Vector<IR::SelectCase> select_cases;

    auto encode_var = cve.state_to_encode_var.at(orig);

    if (encode_var) {
        unsigned var_bitwidth = encode_var->type->width_bits();
        unsigned var_mask = (1 << var_bitwidth) - 1;

        for (auto &ms : length_to_branch_state) {
            auto length = ms.first;
            auto matches = length_to_match.at(length);
            auto next_state = ms.second;

            for (const auto &match : merge_matches(matches, var_mask)) {
                auto select_case =
                    create_select_case(var_bitwidth, match.value, match.mask, next_state->name);
                select_cases.push_back(select_case);
            }
        }

        for (const auto &rej : merge_matches(cve.state_to_reject_matches.at(orig), var_mask)) {
            auto select_case = create_select_case(var_bitwidth, rej.value, rej.mask, "reject"_cs);
            select_cases.push_back(select_case);
        }

        IR::Vector<IR::Expression> select_on;
        select_on.push_back(encode_var);

        tcam_row_usage_estimation += select_cases.size();

        auto select = new IR::SelectExpression(new IR::ListExpression(select_on), select_cases);
        state->selectExpression = select;
    } else {  // unconditional transition (varsize expr is constant)
        BUG_CHECK(length_to_branch_state.size() == 1, "more than one unconditional branch?");

        auto next_state = length_to_branch_state.begin()->second;
        auto path = new IR::PathExpression(next_state->name);
        state->selectExpression = path;
    }

    return true;
}

bool RewriteVarbitUses::preorder(IR::MethodCallExpression *call) {
    auto state = findContext<IR::ParserState>();
    if (state) {
        if (auto method = call->method->to<IR::Member>()) {
            if (method->member == "extract") {
                if (call->arguments->size() == 2) {
                    auto dest = (*call->arguments)[0];
                    auto args = new IR::Vector<IR::Argument>();
                    args->push_back(dest);
                    call->arguments = args;
                }
            }
        }
    }

    return true;
}

static IR::MethodCallStatement *create_emit_statement(const IR::Member *method,
                                                      const IR::Type *type,
                                                      const IR::Expression *path, cstring header) {
    auto typeArgs = new IR::Vector<IR::Type>({type});
    auto args =
        new IR::Vector<IR::Argument>({new IR::Argument(new IR::Member(type, path, header))});
    auto call = new IR::MethodCallExpression(method, typeArgs, args);
    auto emit = new IR::MethodCallStatement(call);
    return emit;
}

IR::AssignmentStatement *create_valid_statement(const IR::Type_Header *hdr,
                                                const IR::Expression *path, bool value) {
    auto hdr_expr = new IR::Member(hdr, path, create_instance_name(hdr->name));
    auto left = new IR::Member(IR::Type::Bits::get(1), hdr_expr, "$valid");
    auto right = new IR::Constant(IR::Type::Bits::get(1), value);
    return new IR::AssignmentStatement(left, right);
}

void RewriteVarbitUses::find_and_replace_setinvalid(const IR::AssignmentStatement *assign,
                                                    IR::IndexedVector<IR::StatOrDecl> &component) {
    auto left = assign->left->to<IR::Member>();
    if (!left || left->member != "$valid") return;
    auto right = assign->right->to<IR::Constant>();
    if (!right) return;
    if (auto expr = left->expr->to<IR::Member>()) {
        if (varbit_hdr_instance_to_header_types.count(expr->member)) {
            if (right->asInt() == 1) {
                error("Cannot set a header %1% with varbit field as valid", expr);
            }
            for (auto &hdr : varbit_hdr_instance_to_header_types.at(expr->member)) {
                component.push_back(create_valid_statement(hdr.second, expr->expr, false));
            }
            auto varbit_field =
                cve.header_type_to_varbit_field.at(expr->type->to<IR::Type_Header>());
            if (varbit_field_to_post_header_type.count(varbit_field)) {
                component.push_back(create_valid_statement(
                    varbit_field_to_post_header_type.at(varbit_field), expr->expr, false));
            }
        }
    }
}

bool RewriteVarbitUses::preorder(IR::BlockStatement *block) {
    auto deparser = findContext<IR::BFN::TnaDeparser>();
    IR::IndexedVector<IR::StatOrDecl> components;
    bool dirty = false;
    for (auto stmt : block->components) {
        components.push_back(stmt);
        if (deparser) {
            if (auto mc = stmt->to<IR::MethodCallStatement>()) {
                auto mce = mc->methodCall;

                if (auto method = mce->method->to<IR::Member>()) {
                    if (method->member == "emit") {
                        auto arg = (*mce->arguments)[0];
                        auto type = arg->expression->type->to<IR::Type_Header>();

                        if (auto *varbit_field = get(cve.header_type_to_varbit_field, type)) {
                            auto path = arg->expression->to<IR::Member>()->expr;
                            auto instance = arg->expression->to<IR::Member>()->member;
                            for (auto [_, header_type] :
                                 get(varbit_hdr_instance_to_header_types, instance)) {
                                auto emit =
                                    create_emit_statement(method, header_type, path,
                                                          create_instance_name(header_type->name));
                                components.push_back(emit);
                            }

                            if (auto *type = get(varbit_field_to_post_header_type, varbit_field)) {
                                auto emit = create_emit_statement(method, type, path,
                                                                  create_instance_name(type->name));
                                components.push_back(emit);
                            }
                            // has varbit, but we did not find any rewritten field
                        } else if (type && has_varbit(type)) {
                            warning(ErrorType::WARN_UNUSED,
                                    "%1%: emitting a header %2% (type %3%) with varbit field "
                                    "that is never parsed in %4%. The header will never be "
                                    "emitted. Consider not emitting this header, or parse it "
                                    "in the corresponding parser. You can also silence the"
                                    "warning by @noWarn(\"unused\") pragma in the header "
                                    "instance declaration.",
                                    arg->expression, arg->expression->toString(), type,
                                    deparser->toString());
                            components.pop_back();
                            dirty = true;
                        }
                    }
                }
            }
        } else {
            // Find setInvalid
            if (auto assign = stmt->to<IR::AssignmentStatement>()) {
                find_and_replace_setinvalid(assign, components);
            }
        }
    }
    if (components.size() || dirty) block->components = components;

    return true;
}

IR::IndexedVector<IR::NamedExpression> RewriteVarbitUses::filter_post_header_fields(
    const IR::IndexedVector<IR::NamedExpression> &components) {
    IR::IndexedVector<IR::NamedExpression> filtered;

    for (auto comp : components) {
        auto c = comp->expression;
        if (auto member = c->to<IR::Member>()) {
            if (auto type = member->expr->type->to<IR::Type_Header>()) {
                if (cve.header_type_to_varbit_field.count(type)) {
                    auto varbit_field = cve.header_type_to_varbit_field.at(type);

                    if (varbit_field_to_post_header_type.count(varbit_field)) {
                        auto post = varbit_field_to_post_header_type.at(varbit_field);

                        bool is_in_post_header = false;

                        for (auto field : post->fields) {
                            if (field->name == member->member) {
                                is_in_post_header = true;
                                break;
                            }
                        }

                        if (is_in_post_header) continue;
                    }
                }
            }
        }

        filtered.push_back(comp);
    }

    return filtered;
}

bool RewriteVarbitUses::preorder(IR::StructExpression *list) {
    auto deparser = findContext<IR::BFN::TnaDeparser>();
    auto mc = findContext<IR::MethodCallExpression>();
    IR::IndexedVector<IR::NamedExpression> components;

    bool has_varbit = false;

    IR::Vector<IR::StructField> varbit_types;

    if (!mc) return false;

    for (auto comp : list->components) {
        auto c = comp->expression;
        if (auto member = c->to<IR::Member>()) {
            if (member->type->is<IR::Type_Varbits>()) {
                if (has_varbit) error("More than one varbit expression in %1%", list);
                if (deparser) {
                    has_varbit = true;
                    auto path = member->expr->to<IR::Member>();

                    unsigned ind = 0;
                    for (auto &kv : varbit_hdr_instance_to_header_types.at(path->member)) {
                        auto hdr = kv.second;
                        auto field = hdr->fields[0];

                        auto new_name = IR::ID(comp->name + "_vbit_" + cstring::to_cstring(ind++));
                        auto st_field = new IR::StructField(new_name, field->type);
                        varbit_types.push_back(st_field);

                        auto member = new IR::Member(path->expr, create_instance_name(hdr->name));
                        auto hdr_field =
                            new IR::NamedExpression(new_name, new IR::Member(member, "field"));

                        components.push_back(hdr_field);
                    }
                } else if (auto state = findOrigCtxt<IR::ParserState>()) {
                    if (cve.state_to_csum_add.count(state)) {
                        for (auto call : cve.state_to_csum_add.at(state)) {
                            if (call->equiv(*mc)) {
                                has_varbit = true;
                                break;
                            }
                        }
                    }
                }

                continue;
            }
        }

        components.push_back(comp);
    }

    if (has_varbit) {
        list->components = components;

        if (auto tuple = list->type->to<IR::Type_StructLike>()) {
            IR::Vector<IR::StructField> types;

            for (auto field : tuple->fields) {
                if (field->type->is<IR::Type_Varbits>()) {
                    for (auto vt : varbit_types) types.push_back(vt);
                } else {
                    types.push_back(field);
                }
            }

            if (mc) {
                auto type_args = *(mc->typeArguments);

                if (type_args.size() == 1) {
                    auto tuple_type = type_args[0];

                    if (auto name = tuple_type->to<IR::Type_Name>()) {
                        auto tuple_name = name->path->name;
                        tuple_types_to_rewrite[tuple_name] = types;

                        bool filter = false;

                        for (auto &kv : cve.header_type_to_varbit_field) {
                            if (kv.first->name == tuple_name) {
                                filter = true;
                                break;
                            }
                        }

                        // if the tuple type is the original header type
                        // we need to remove the fields after the varbit field from the
                        // tuple as they have been moved into its own header type and
                        // instance

                        if (filter) list->components = filter_post_header_fields(components);
                    }
                } else if (type_args.size() > 1) {
                    error("More than one type in %1%", list);
                }
            }
        }
    }

    return true;
}

bool RewriteVarbitUses::preorder(IR::Member *member) {
    if (auto type = member->expr->type->to<IR::Type_Header>()) {
        if (cve.header_type_to_varbit_field.count(type)) {
            auto varbit_field = cve.header_type_to_varbit_field.at(type);

            if (varbit_field_to_post_header_type.count(varbit_field)) {
                auto post = varbit_field_to_post_header_type.at(varbit_field);

                bool is_in_post_header = false;

                for (auto field : post->fields) {
                    if (field->name == member->member) {
                        is_in_post_header = true;
                        break;
                    }
                }

                // rewrite path to fields that are after the varbit field
                // these fields have been moved into its own header type and instance

                if (is_in_post_header) {
                    auto path = member->expr->to<IR::Member>();
                    auto post_hdr = varbit_field_to_post_header_type.at(varbit_field);
                    auto post_hdr_inst = create_instance_name(post_hdr->name);
                    member->expr = new IR::Member(path->expr, post_hdr_inst);
                }
            }
        }
    }

    return true;
}

/**
 * \ingroup DesugarVarbitExtract
 */
class RewriteParserVerify : public Transform {
    const CollectVarbitExtract &cve;

    IR::Node *preorder(IR::MethodCallStatement *) override;

 public:
    explicit RewriteParserVerify(const CollectVarbitExtract &cve) : cve(cve) {}
};

IR::Node *RewriteParserVerify::preorder(IR::MethodCallStatement *stmt) {
    auto state = findContext<IR::ParserState>();
    if (state) {
        auto call = stmt->methodCall;

        if (auto path = call->method->to<IR::PathExpression>()) {
            if (path->path->name == "verify") {
                auto verify_expr = (*call->arguments)[0]->expression;

                CollectVariables find_var;
                verify_expr->apply(find_var);

                if (find_var.rv.size() == 1) {
                    for (auto &sv : cve.state_to_encode_var) {
                        if (sv.second->equiv(*(find_var.rv[0]))) {
                            LOG4("remove " << stmt << " absorbed in varbit rewrite");
                            return nullptr;
                        }
                    }
                }
            }
        }
    }

    return stmt;
}

/**
 * \ingroup DesugarVarbitExtract
 */
struct RemoveZeroVarbitExtract : public Modifier {
    bool preorder(IR::ParserState *state) override {
        IR::IndexedVector<IR::StatOrDecl> rv;

        for (auto stmt : state->components) {
            if (auto mc = stmt->to<IR::MethodCallStatement>()) {
                auto call = mc->methodCall;
                if (auto method = call->method->to<IR::Member>()) {
                    if (method->member == "extract") {
                        if (call->arguments->size() == 2) {
                            auto varsize_expr = (*call->arguments)[1]->expression;
                            if (auto c = varsize_expr->to<IR::Constant>()) {
                                if (c->asInt() == 0) continue;
                            }
                        }
                    }
                }
            }

            rv.push_back(stmt);
        }

        state->components = rv;
        return false;
    }
};

/**
 * \ingroup DesugarVarbitExtract
 */
class RewriteVarbitTypes : public Modifier {
    const CollectVarbitExtract &cve;
    const RewriteVarbitUses &rvu;

    bool contains_varbit_header(IR::Type_StructLike *);

    bool preorder(IR::P4Program *) override;
    bool preorder(IR::Type_StructLike *) override;
    bool preorder(IR::Type_Header *) override;

 public:
    RewriteVarbitTypes(const CollectVarbitExtract &c, const RewriteVarbitUses &r)
        : cve(c), rvu(r) {}
};

bool RewriteVarbitTypes::preorder(IR::P4Program *program) {
    for (auto &kv : rvu.varbit_hdr_instance_to_header_types) {
        for (auto &lt : kv.second) program->objects.insert(program->objects.begin(), lt.second);
    }

    for (auto &kv : rvu.varbit_field_to_post_header_type)
        program->objects.insert(program->objects.begin(), kv.second);

    return true;
}

bool RewriteVarbitTypes::contains_varbit_header(IR::Type_StructLike *type_struct) {
    for (auto field : type_struct->fields) {
        if (auto path = field->type->to<IR::Type_Name>()) {
            for (auto &kv : cve.header_type_to_varbit_field) {
                if (kv.first->name == path->path->name) return true;
            }
        }
    }

    return false;
}

bool RewriteVarbitTypes::preorder(IR::Type_StructLike *type_struct) {
    if (type_struct->is<IR::Type_HeaderUnion>()) {
        fatal_error(ErrorType::ERR_UNSUPPORTED_ON_TARGET, "Unsupported type %s", type_struct);
    }

    if (contains_varbit_header(type_struct) && type_struct->name.name.find("tuple_") == nullptr) {
        for (auto &kv : rvu.varbit_hdr_instance_to_header_types) {
            for (auto &lt : kv.second) {
                auto type = lt.second;
                auto field = new IR::StructField(create_instance_name(type->name),
                                                 new IR::Type_Name(type->name));
                type_struct->fields.push_back(field);
                LOG3("Adding " << field << " in " << type_struct);
            }
        }

        for (auto &kv : rvu.varbit_field_to_post_header_type) {
            auto type = kv.second;
            auto field = new IR::StructField(create_instance_name(type->name),
                                             new IR::Type_Name(type->name));
            type_struct->fields.push_back(field);
            LOG3("Adding " << field << "in " << type_struct);
        }
    } else {
        for (auto &kv : rvu.tuple_types_to_rewrite) {
            if (type_struct->name == kv.first) {
                type_struct->fields = {};

                for (auto field : kv.second) {
                    type_struct->fields.push_back(field);
                    LOG3("Adding " << field << "in " << type_struct);
                }
            }
        }
    }

    return true;
}

bool RewriteVarbitTypes::preorder(IR::Type_Header *header) {
    auto orig = getOriginal<IR::Type_Header>();

    if (cve.header_type_to_varbit_field.count(orig)) {
        IR::IndexedVector<IR::StructField> fields;

        for (auto field : header->fields) {
            if (field->type->is<IR::Type::Varbits>()) break;

            fields.push_back(field);
        }

        header->fields = fields;
    }

    return true;
}

CheckVarbitAccess::CheckVarbitAccess() { addPasses({new ErrorOnUnsupportedVarbitUse}); }

DesugarVarbitExtract::DesugarVarbitExtract(P4::ReferenceMap *refMap, P4::TypeMap *typeMap) {
    auto remove_zero_varbit = new RemoveZeroVarbitExtract;
    auto collect_varbit_extract = new CollectVarbitExtract(refMap, typeMap);
    auto rewrite_varbit_uses = new RewriteVarbitUses(*collect_varbit_extract);
    auto rewrite_parser_verify = new RewriteParserVerify(*collect_varbit_extract);
    auto rewrite_varbit_types =
        new RewriteVarbitTypes(*collect_varbit_extract, *rewrite_varbit_uses);

    addPasses({remove_zero_varbit, collect_varbit_extract, rewrite_varbit_uses,
               rewrite_parser_verify, rewrite_varbit_types, new P4::CloneExpressions,
               new P4::ClearTypeMap(typeMap), new BFN::TypeChecking(refMap, typeMap, true)});
}

}  // namespace BFN
