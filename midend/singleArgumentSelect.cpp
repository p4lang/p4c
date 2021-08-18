#include "singleArgumentSelect.h"

namespace P4 {

DoSingleArgumentSelect::Pair::Pair(const IR::Expression* e, const IR::Type* type) {
    auto srcInfo = e->srcInfo;
    if (auto m = e->to<IR::Mask>()) {
        expr = m->left;
        mask = m->right;
        hasMask = true;
    } else if (e->is<IR::DefaultExpression>()) {
        expr = new IR::Constant(srcInfo, type, 0, 16);
        mask = new IR::Constant(srcInfo, type, 0, 16);
        hasMask = true;
    } else {
        expr = e;
        mask = new IR::Constant(srcInfo, type, Util::maskFromSlice(type->width_bits()-1, 0), 16);
        hasMask = false;
    }
}

static const IR::Expression* convertList(
    const IR::Expression* expression, const IR::Type* selectListType) {
    if (expression->is<IR::DefaultExpression>()) {
        int width = selectListType->width_bits();
        auto type = new IR::Type_Bits(width, false);
        return new IR::Mask(expression->srcInfo,
                            new IR::Constant(type, 0, 16),
                            new IR::Constant(type, 0, 16));
    }
    auto list = expression->to<IR::ListExpression>();
    if (list == nullptr)
        return expression;
    BUG_CHECK(list->components.size() > 0, "%1%: No components", list);
    auto tt = selectListType->checkedTo<IR::Type_List>();
    BUG_CHECK(list->size() == tt->components.size(),
              "%1% and %2% do not have the same size",
              list, tt);
    auto p = new DoSingleArgumentSelect::Pair(list->components.at(0), tt->components.at(0));
    auto hasMask = p->hasMask;
    auto expr = p->expr;
    auto mask = p->mask;
    for (size_t i = 1; i < list->components.size(); i++) {
        p = new DoSingleArgumentSelect::Pair(list->components.at(i), tt->components.at(i));
        expr = new IR::Concat(expr, p->expr);
        mask = new IR::Concat(mask, p->mask);
        hasMask = hasMask || p->hasMask;
    }
    if (!hasMask)
        return expr;
    else
        return new IR::Mask(expression->srcInfo, expr, mask);
}

bool DoSingleArgumentSelect::preorder(IR::SelectExpression* expression) {
    selectListType = typeMap->getType(expression->select, true);
    auto conv = convertList(expression->select, selectListType);
    auto list = new IR::ListExpression(IR::Vector<IR::Expression>());
    list->push_back(conv);
    expression->select = list;
    return true;
}

bool DoSingleArgumentSelect::preorder(IR::SelectCase* selCase) {
    selCase->keyset = convertList(selCase->keyset, selectListType);
    return false;
}

}  // namespace P4
