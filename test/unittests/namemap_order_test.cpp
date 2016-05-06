#include "ir/ir.h"
#include "ir/visitor.h"
#include <assert.h>

// THIS TEST IS BROKEN, since TableProperties no longer uses a NameMap.

class TestTrans : public Transform {
    IR::Node *preorder(IR::TableProperty *a) override {
        if (!a->isConstant)
            a->name = "$new$";
        return a;
    }
    IR::Node *postorder(IR::TableProperties *p) override {
        int count = 0;
        for (auto &prop : *p->properties) {
            if (prop == "$new$")
                assert(count == 0);
            else
                assert(count == 1);
            ++count; }
        return p;
    }
};

int main() {
    auto tp = new IR::TableProperties();
    tp->properties.add("a", new IR::TableProperty(Util::SourceInfo(), "a",
        new IR::Annotations(new IR::Vector<IR::Annotation>()),
        new IR::Key(Util::SourceInfo(), new IR::Vector<IR::KeyElement>()),
        false));
    tp->properties.add("b", new IR::TableProperty(Util::SourceInfo(), "b",
        new IR::Annotations(new IR::Vector<IR::Annotation>()),
        new IR::Key(Util::SourceInfo(), new IR::Vector<IR::KeyElement>()),
        true));
    tp->apply(TestTrans());
}
