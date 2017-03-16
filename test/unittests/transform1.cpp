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
