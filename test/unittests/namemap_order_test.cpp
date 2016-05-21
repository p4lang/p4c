/*
Copyright 2013-present Barefoot Networks, Inc. 

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

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
