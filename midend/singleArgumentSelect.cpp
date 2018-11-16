#include "singleArgumentSelect.h"

namespace P4 {

static const IR::Expression* convertList(const IR::Expression* expression) {
    auto list = expression->to<IR::ListExpression>();
    if (list == nullptr)
        return expression;
    BUG_CHECK(list->components.size() > 0, "%1%: No components", list);
    auto expr = list->components.at(0);
    for (size_t i = 1; i < list->components.size(); i++)
        expr = new IR::Concat(expr, list->components.at(i));
    return expr;
}

bool SingleArgumentSelect::preorder(IR::SelectExpression* expression) {
    if (expression->select->size() <= 1)
        return false;
    auto conv = convertList(expression->select);
    auto list = new IR::ListExpression(IR::Vector<IR::Expression>());
    list->push_back(conv);
    expression->select = list;
    return true;
}

bool SingleArgumentSelect::preorder(IR::SelectCase* selCase) {
    selCase->keyset = convertList(selCase->keyset);
    return false;
}

}  // namespace P4
