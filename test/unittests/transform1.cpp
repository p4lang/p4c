#include "ir/ir.h"
#include "ir/visitor.h"
#include <assert.h>

static IR::Constant *c1;

class TestTrans : public Transform {
    IR::Node *postorder(IR::Add *a) override {
        assert(a->left == c1);
        assert(a->right == c1);
        return a;
    }
};

int main() {
    c1 = new IR::Constant(2);
    IR::Expression *e = new IR::Add(Util::SourceInfo(), c1, c1);
    auto *n = e->apply(TestTrans());
    assert(n == e);
}
