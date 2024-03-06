#include "backends/p4tools/modules/smith/common/scope.h"

#include <algorithm>
#include <cstddef>
#include <utility>
#include <vector>

#include "backends/p4tools/modules/smith/common/expressions.h"
namespace P4Tools {

namespace P4Smith {

std::vector<IR::Vector<IR::Node> *> P4Scope::scope;
std::set<cstring> P4Scope::used_names;
std::map<cstring, std::map<int, std::set<cstring>>> P4Scope::lval_map;
std::map<cstring, std::map<int, std::set<cstring>>> P4Scope::lval_map_rw;
std::set<const IR::P4Table *> P4Scope::callable_tables;

// TODO: This should be set by the back end
std::set<cstring> P4Scope::not_initialized_structs;
Properties P4Scope::prop;
Requirements P4Scope::req;
Constraints P4Scope::constraints;
void P4Scope::add_to_scope(const IR::Node *node) {
    auto l_scope = P4Scope::scope.back();
    l_scope->push_back(node);

    if (auto dv = node->to<IR::Declaration_Variable>()) {
        add_lval(dv->type, dv->name.name);
    } else if (auto dc = node->to<IR::Declaration_Constant>()) {
        add_lval(dc->type, dc->name.name, true);
    }
}

void P4Scope::start_local_scope() {
    IR::Vector<IR::Node> *local_scope = new IR::Vector<IR::Node>();
    scope.push_back(local_scope);
}

void P4Scope::end_local_scope() {
    IR::Vector<IR::Node> *local_scope = scope.back();

    for (auto node : *local_scope) {
        if (auto decl = node->to<IR::Declaration_Variable>()) {
            delete_lval(decl->type, decl->name.name);
        } else if (auto dc = node->to<IR::Declaration_Constant>()) {
            delete_lval(dc->type, dc->name.name);
        } else if (auto param = node->to<IR::Parameter>()) {
            delete_lval(param->type, param->name.name);
        } else if (auto tbl = node->to<IR::P4Table>()) {
            callable_tables.erase(tbl);
        }
    }

    delete local_scope;
    scope.pop_back();
}

void add_compound_lvals(const IR::Type_StructLike *sl_type, cstring sl_name, bool read_only) {
    for (auto field : sl_type->fields) {
        std::stringstream ss;
        ss.str("");
        ss << sl_name << "." << field->name.name;
        cstring field_name(ss.str());
        P4Scope::add_lval(field->type, field_name, read_only);
    }
}

void delete_compound_lvals(const IR::Type_StructLike *sl_type, cstring sl_name) {
    for (auto field : sl_type->fields) {
        std::stringstream ss;
        ss.str("");
        ss << sl_name << "." << field->name.name;
        cstring field_name(ss.str());
        P4Scope::delete_lval(field->type, field_name);
    }
}

void P4Scope::delete_lval(const IR::Type *tp, cstring name) {
    cstring type_key;
    int bit_bucket;

    if (auto tb = tp->to<IR::Type_Bits>()) {
        type_key = IR::Type_Bits::static_type_name();
        bit_bucket = tb->width_bits();
    } else if (auto tb = tp->to<IR::Type_Boolean>()) {
        type_key = IR::Type_Boolean::static_type_name();
        bit_bucket = tb->width_bits();
    } else if (tp->is<IR::Type_InfInt>()) {
        type_key = IR::Type_InfInt::static_type_name();
        bit_bucket = 1;
    } else if (auto ts = tp->to<IR::Type_Stack>()) {
        size_t stack_size = ts->getSize();
        type_key = IR::Type_Stack::static_type_name();
        bit_bucket = 1;
        if (auto tn_type = ts->elementType->to<IR::Type_Name>()) {
            std::stringstream ss;
            ss.str("");
            ss << name << "[" << (stack_size - 1) << "]";
            cstring stack_name(ss.str());
            delete_lval(tn_type, stack_name);
        } else {
            BUG("Type_Name %s not yet supported", ts->elementType->node_type_name());
        }
    } else if (auto tn = tp->to<IR::Type_Name>()) {
        auto tn_name = tn->path->name.name;
        if (tn_name == "packet_in") {
            return;
        }
        if (auto td = get_type_by_name(tn_name)) {
            if (auto tn_type = td->to<IR::Type_StructLike>()) {
                type_key = tn_name;
                // width_bits should work here, do !know why not...
                // bit_bucket = P4Scope::compound_type[tn_name]->width_bits();
                bit_bucket = 1;
                delete_compound_lvals(tn_type, name);
            } else {
                BUG("Type_Name %s !found!", td->node_type_name());
            }
        } else {
            printf("Type %s does not exist!\n", tn_name.c_str());
            return;
        }
    } else {
        BUG("Type %s not yet supported", tp->node_type_name());
    }
    lval_map[type_key][bit_bucket].erase(name);

    // delete values in the normal map
    if (lval_map[type_key][bit_bucket].size() == 0) {
        lval_map[type_key].erase(bit_bucket);
    }

    // delete values in the rw map
    if (lval_map_rw.count(type_key) != 0) {
        if (lval_map_rw[type_key].count(bit_bucket) != 0) {
            lval_map_rw[type_key][bit_bucket].erase(name);
            if (lval_map_rw[type_key][bit_bucket].size() == 0) {
                lval_map_rw[type_key].erase(bit_bucket);
            }
        }
    }
    // delete empty type entries
    if (lval_map[type_key].size() == 0) {
        lval_map.erase(type_key);
    }
    if (lval_map_rw[type_key].size() == 0) {
        lval_map_rw.erase(type_key);
    }
}

void P4Scope::add_lval(const IR::Type *tp, cstring name, bool read_only) {
    cstring type_key;
    int bit_bucket;

    if (auto tb = tp->to<IR::Type_Bits>()) {
        type_key = IR::Type_Bits::static_type_name();
        bit_bucket = tb->width_bits();
    } else if (auto tb = tp->to<IR::Type_Boolean>()) {
        type_key = IR::Type_Boolean::static_type_name();
        bit_bucket = tb->width_bits();
    } else if (tp->is<IR::Type_InfInt>()) {
        type_key = IR::Type_InfInt::static_type_name();
        bit_bucket = 1;
    } else if (auto ts = tp->to<IR::Type_Stack>()) {
        size_t stack_size = ts->getSize();
        type_key = IR::Type_Stack::static_type_name();
        bit_bucket = 1;
        if (auto tn_type = ts->elementType->to<IR::Type_Name>()) {
            std::stringstream ss;
            ss.str("");
            ss << name << "[" << (stack_size - 1) << "]";
            cstring stack_name(ss.str());
            add_lval(tn_type, stack_name, read_only);
        } else {
            BUG("Type_Name %s not yet supported", ts->elementType->node_type_name());
        }
    } else if (auto tn = tp->to<IR::Type_Name>()) {
        auto tn_name = tn->path->name.name;
        // FIXME: this could be better
        if (tn_name == "packet_in") {
            return;
        }
        if (auto td = get_type_by_name(tn_name)) {
            if (auto tn_type = td->to<IR::Type_StructLike>()) {
                // width_bits should work here, do !know why not...
                type_key = tn_name;
                // does !work for some reason...
                // bit_bucket = P4Scope::compound_type[tn_name]->width_bits();
                bit_bucket = 1;
                add_compound_lvals(tn_type, name, read_only);
            } else {
                BUG("Type_Name %s not yet supported", td->node_type_name());
            }
        } else {
            BUG("Type %s does not exist", tn_name);
        }
    } else {
        BUG("Type %s not yet supported", tp->node_type_name());
    }
    if (!read_only) {
        lval_map_rw[type_key][bit_bucket].insert(name);
    }
    lval_map[type_key][bit_bucket].insert(name);
}

std::set<cstring> P4Scope::get_candidate_lvals(const IR::Type *tp, bool must_write) {
    cstring type_key;
    int bit_bucket;

    if (auto tb = tp->to<IR::Type_Bits>()) {
        type_key = IR::Type_Bits::static_type_name();
        bit_bucket = tb->width_bits();
    } else if (auto tb = tp->to<IR::Type_Boolean>()) {
        type_key = IR::Type_Boolean::static_type_name();
        bit_bucket = tb->width_bits();
    } else if (tp->is<IR::Type_InfInt>()) {
        type_key = IR::Type_InfInt::static_type_name();
        bit_bucket = 1;
    } else if (auto ts = tp->to<IR::Type_StructLike>()) {
        type_key = ts->name.name;
        bit_bucket = 1;
    } else if (auto tn = tp->to<IR::Type_Name>()) {
        auto tn_name = tn->path->name.name;
        if (get_type_by_name(tn_name)) {
            type_key = tn_name;
            // bit_bucket = td->width_bits();
            bit_bucket = 1;
        } else {
            BUG("Type_name refers to unknown type %s", tn_name);
        }
    } else {
        BUG("Type %s not yet supported", tp->node_type_name());
    }

    std::map<cstring, std::map<int, std::set<cstring>>> lookup_map;

    if (must_write) {
        lookup_map = lval_map_rw;
    } else {
        lookup_map = lval_map;
    }

    if (lookup_map.count(type_key) == 0) {
        return {};
    }
    auto key_types = lookup_map[type_key];
    if (key_types.count(bit_bucket) == 0) {
        return {};
    }

    return key_types[bit_bucket];
}

bool P4Scope::check_lval(const IR::Type *tp, bool must_write) {
    std::set<cstring> candidates = get_candidate_lvals(tp, must_write);

    if (candidates.empty()) {
        return false;
    }
    return true;
}

cstring P4Scope::pick_lval(const IR::Type *tp, bool must_write) {
    std::set<cstring> candidates = get_candidate_lvals(tp, must_write);

    if (candidates.empty()) {
        BUG("Invalid Type Query, %s not found", tp->toString());
    }
    size_t idx = getRndInt(0, candidates.size() - 1);
    auto lval = std::begin(candidates);
    // "advance" the iterator idx times
    std::advance(lval, idx);

    return *lval;
}

size_t split(const std::string &txt, std::vector<cstring> &strs, char ch) {
    size_t pos = txt.find(ch);
    size_t initialPos = 0;
    strs.clear();

    // Decompose statement
    while (pos != std::string::npos) {
        strs.push_back(txt.substr(initialPos, pos - initialPos));
        initialPos = pos + 1;

        pos = txt.find(ch, initialPos);
    }

    // Add the last one
    strs.push_back(txt.substr(initialPos, std::min(pos, txt.size()) - initialPos + 1));

    return strs.size();
}

IR::Expression *edit_hdr_stack(cstring lval) {
    // Check if there is a stack bracket inside the string.
    // Also, if we can not have variables inside the header stack index,
    // then just return the original expression.
    // FIXME: terrible but at least works for now
    if (!lval.find('[') || P4Scope::constraints.const_header_stack_index) {
        return new IR::PathExpression(lval);
    }

    std::vector<cstring> split_str;
    int size = split(lval.c_str(), split_str, '.');
    if (size < 1) {
        BUG("Unexpected split size. %d", size);
    }
    auto s_iter = std::begin(split_str);
    IR::Expression *expr = new IR::PathExpression(*s_iter);
    for (advance(s_iter, 1); s_iter != split_str.end(); ++s_iter) {
        // if there is an index, convert it towards the proper expression
        auto sub_str = *s_iter;
        auto hdr_brkt = sub_str.find('[');
        if (hdr_brkt) {
            auto stack_str = sub_str.substr(static_cast<size_t>(hdr_brkt - sub_str + 1));
            auto stack_sz_end = stack_str.find(']');
            if (!stack_sz_end) {
                BUG("There should be a closing bracket.");
            }
            int stack_sz = std::stoi(stack_str.before(stack_sz_end).c_str());
            expr = new IR::Member(expr, sub_str.before(hdr_brkt));
            auto tb = new IR::Type_Bits(3, false);
            IR::Expression *idx = Expressions().genExpression(tb);
            IR::Vector<IR::Argument> *args = new IR::Vector<IR::Argument>();
            args->push_back(new IR::Argument(idx));
            args->push_back(new IR::Argument(new IR::Constant(tb, stack_sz)));
            idx = new IR::MethodCallExpression(new IR::PathExpression("max"), args);

            expr = new IR::ArrayIndex(expr, idx);
        } else {
            expr = new IR::Member(expr, sub_str);
        }
    }
    return expr;
}

IR::Expression *P4Scope::pick_lval_or_slice(const IR::Type *tp) {
    cstring lval_str = pick_lval(tp, true);
    IR::Expression *expr = edit_hdr_stack(lval_str);

    if (auto tb = tp->to<IR::Type_Bits>()) {
        std::vector<int64_t> percent = {PCT.SCOPE_LVAL_PATH, PCT.SCOPE_LVAL_SLICE};
        switch (randInt(percent)) {
            case 0: {
                break;
            }
            case 1: {
                cstring type_key = IR::Type_Bits::static_type_name();
                if (lval_map_rw.count(type_key) == 0) {
                    break;
                }
                auto key_types = lval_map_rw[type_key];
                size_t target_width = tb->width_bits();

                std::vector<std::pair<size_t, cstring>> candidates;

                for (auto bit_bucket : key_types) {
                    size_t key_size = bit_bucket.first;
                    if (key_size > target_width) {
                        for (cstring lval : key_types[key_size]) {
                            candidates.emplace_back(key_size, lval);
                        }
                    }
                }
                if (candidates.empty()) {
                    break;
                }
                size_t idx = getRndInt(0, candidates.size() - 1);
                auto lval = std::begin(candidates);
                // "advance" the iterator idx times
                std::advance(lval, idx);
                size_t candidate_size = lval->first;
                auto slice_expr = new IR::PathExpression(lval->second);
                size_t low = getRndInt(0, candidate_size - target_width);
                size_t high = low + target_width - 1;
                expr = new IR::Slice(slice_expr, high, low);
            } break;
        }
    }
    return expr;
}

IR::Type_Bits *P4Scope::pick_declared_bit_type(bool must_write) {
    std::map<cstring, std::map<int, std::set<cstring>>> lookup_map;

    if (must_write) {
        lookup_map = lval_map_rw;
    } else {
        lookup_map = lval_map;
    }

    cstring bit_key = IR::Type_Bits::static_type_name();
    if (lookup_map.count(bit_key) == 0) {
        return nullptr;
    }
    auto key_types = lookup_map[bit_key];
    size_t idx = getRndInt(0, key_types.size() - 1);
    int bit_width = next(key_types.begin(), idx)->first;
    return new IR::Type_Bits(bit_width, false);
}

std::vector<const IR::Type_Declaration *> P4Scope::get_filtered_decls(std::set<cstring> filter) {
    std::vector<const IR::Type_Declaration *> ret;
    for (auto sub_scope : scope) {
        for (auto node : *sub_scope) {
            cstring name = node->node_type_name();
            if (filter.find(name) == filter.end()) {
                if (auto decl = node->to<IR::Type_Declaration>()) {
                    ret.push_back(decl);
                }
            }
        }
    }
    return ret;
}

std::set<const IR::P4Table *> *P4Scope::get_callable_tables() { return &callable_tables; }

const IR::Type_Declaration *P4Scope::get_type_by_name(cstring name) {
    for (auto sub_scope : scope) {
        for (auto node : *sub_scope) {
            if (auto decl = node->to<IR::Type_Declaration>()) {
                if (decl->name.name == name) {
                    return decl;
                }
            }
        }
    }
    return nullptr;
}
}  // namespace P4Smith

}  // namespace P4Tools
