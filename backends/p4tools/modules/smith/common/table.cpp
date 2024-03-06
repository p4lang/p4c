#include "backends/p4tools/modules/smith/common/table.h"

#include "backends/p4tools/modules/smith/common/declarations.h"
#include "backends/p4tools/modules/smith/common/expressions.h"
#include "backends/p4tools/modules/smith/common/scope.h"
#include "backends/p4tools/modules/smith/util/util.h"

namespace P4Tools::P4Smith {

IR::P4Table *Table::genTableDeclaration() {
    IR::TableProperties *tb_properties = genTablePropertyList();
    cstring name = randStr(6);
    auto ret = new IR::P4Table(name, tb_properties);
    P4Scope::add_to_scope(ret);
    P4Scope::callable_tables.emplace(ret);
    return ret;
}

IR::TableProperties *Table::genTablePropertyList() {
    IR::IndexedVector<IR::Property> tab_properties;

    tab_properties.push_back(genKeyProperty());
    tab_properties.push_back(genActionListProperty());

    return new IR::TableProperties(tab_properties);
}

IR::Key *Table::genKeyElementList(size_t len) {
    IR::Vector<IR::KeyElement> keys;

    for (size_t i = 0; i < len; i++) {
        // TODO: More types than just exact
        IR::KeyElement *key = genKeyElement("exact");
        if (!key) {
            continue;
        }
        // @name
        // Tao: actually, this may never happen
        auto key_anno = key->annotations->annotations.at(0);
        auto annot_expr = key_anno->expr.at(0);
        cstring key_annotat_name;
        if (annot_expr->is<IR::StringLiteral>()) {
            auto str_expr = annot_expr->to<IR::StringLiteral>();
            key_annotat_name = str_expr->value;
        } else {
            BUG("must be a string literal");
        }

        keys.push_back(key);
    }

    return new IR::Key(keys);
}

IR::KeyElement *Table::genKeyElement(cstring match_kind) {
    auto match = new IR::PathExpression(match_kind);
    auto annotations = Declarations().genAnnotation();
    auto bit_type = P4Scope::pick_declared_bit_type(false);

    // Ideally this should have a fallback option
    if (!bit_type) {
        printf("Could not find key lval for key matches\n");
        return nullptr;
    }
    // this expression can!be an infinite precision integer
    P4Scope::req.require_scalar = true;
    auto expr = Expressions().genExpression(bit_type);
    P4Scope::req.require_scalar = false;
    auto key = new IR::KeyElement(annotations, expr, match);

    return key;
}

IR::Property *Table::genKeyProperty() {
    cstring name = IR::TableProperties::keyPropertyName;
    auto keys = genKeyElementList(getRndInt(0, 3));

    // isConstant --> false
    return new IR::Property(name, keys, false);
}

IR::MethodCallExpression *Table::genTableActionCall(cstring method_name, IR::ParameterList params) {
    IR::Vector<IR::Argument> *args = new IR::Vector<IR::Argument>();
    IR::IndexedVector<IR::StatOrDecl> decls;

    bool can_call = true;

    for (auto par : params) {
        if (Expressions().checkInputArg(par)) {
            can_call = false;
        } else if (par->direction == IR::Direction::None) {
            // do nothing; in tables directionless parameters are
            // set by the control plane
            continue;
        } else {
            IR::Argument *arg;
            if (par->direction == IR::Direction::In) {
                // the generated expression needs to be compile-time known
                P4Scope::req.compile_time_known = true;
                arg = new IR::Argument(Expressions().genExpression(par->type));
                P4Scope::req.compile_time_known = false;
            } else {
                arg = new IR::Argument(P4Scope::pick_lval_or_slice(par->type));
            }
            args->push_back(arg);
        }
    }
    if (can_call) {
        auto path_expr = new IR::PathExpression(method_name);
        return new IR::MethodCallExpression(path_expr, args);
    } else {
        delete args;
        return nullptr;
    }
}

IR::ActionList *Table::genActionList(size_t len) {
    IR::IndexedVector<IR::ActionListElement> act_list;
    auto p4_actions = P4Scope::get_decls<IR::P4Action>();
    std::set<cstring> act_names;

    if (p4_actions.size() == 0) {
        return new IR::ActionList(act_list);
    }
    for (size_t i = 0; i < len; i++) {
        size_t idx = getRndInt(0, p4_actions.size() - 1);
        auto p4_act = p4_actions[idx];
        cstring act_name = p4_act->name.name;

        if (act_names.find(act_name) != act_names.end()) {
            continue;
        }
        act_names.insert(act_name);

        auto params = p4_act->parameters;
        IR::MethodCallExpression *mce = genTableActionCall(act_name, *params);
        if (mce) {
            IR::ActionListElement *actlist_ele = new IR::ActionListElement(mce);
            act_list.push_back(actlist_ele);
        }
    }
    return new IR::ActionList(act_list);
}

IR::Property *Table::genActionListProperty() {
    cstring name = IR::TableProperties::actionsPropertyName;
    auto acts = genActionList(getRndInt(0, 3));

    return new IR::Property(name, acts, false);
}

}  // namespace P4Tools::P4Smith
