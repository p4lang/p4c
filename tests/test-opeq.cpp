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

#include <assert.h>
#include "ir/ir.h"
#include "ir/visitor.h"
#include "lib/exceptions.h"
#include "p4ctest.h"

static void test_opeq_main(int, char*[]) {
    auto *t = IR::Type::Bits::get(16);
    IR::Constant *a = new IR::Constant(t, 10);
    IR::Constant *b = new IR::Constant(t, 10);
    IR::Constant *c = new IR::Constant(t, 20);

    assert(*a == *b);
    assert(*a == *static_cast<IR::Expression *>(b));
    assert(*a == *static_cast<IR::Node *>(b));
    assert(*static_cast<IR::Expression *>(a) == *b);
    assert(*static_cast<IR::Expression *>(a) == *static_cast<IR::Expression *>(b));
    assert(*static_cast<IR::Expression *>(a) == *static_cast<IR::Node *>(b));
    assert(*static_cast<IR::Node *>(a) == *b);
    assert(*static_cast<IR::Node *>(a) == *static_cast<IR::Expression *>(b));
    assert(*static_cast<IR::Node *>(a) == *static_cast<IR::Node *>(b));

    assert(!(*a == *c));
    assert(!(*a == *static_cast<IR::Expression *>(c)));
    assert(!(*a == *static_cast<IR::Node *>(c)));
    assert(!(*static_cast<IR::Expression *>(a) == *c));
    assert(!(*static_cast<IR::Expression *>(a) == *static_cast<IR::Expression *>(c)));
    assert(!(*static_cast<IR::Expression *>(a) == *static_cast<IR::Node *>(c)));
    assert(!(*static_cast<IR::Node *>(a) == *c));
    assert(!(*static_cast<IR::Node *>(a) == *static_cast<IR::Expression *>(c)));
    assert(!(*static_cast<IR::Node *>(a) == *static_cast<IR::Node *>(c)));

    auto *p1 = new IR::IndexedVector<IR::Node>(a);
    auto *p2 = p1->clone();
    auto *p3 = new IR::IndexedVector<IR::Node>(c);

    assert(*p1 == *p2);
    assert(*p1 == *static_cast<IR::Vector<IR::Node> *>(p2));
    assert(*p1 == *static_cast<IR::Node *>(p2));
    assert(*static_cast<IR::Vector<IR::Node> *>(p1) == *p2);
    assert(*static_cast<IR::Vector<IR::Node> *>(p1) == *static_cast<IR::Vector<IR::Node> *>(p2));
    assert(*static_cast<IR::Vector<IR::Node> *>(p1) == *static_cast<IR::Node *>(p2));
    assert(*static_cast<IR::Node *>(p1) == *p2);
    assert(*static_cast<IR::Node *>(p1) == *static_cast<IR::Vector<IR::Node> *>(p2));
    assert(*static_cast<IR::Node *>(p1) == *static_cast<IR::Node *>(p2));

    assert(!(*p1 == *p3));
    assert(!(*p1 == *static_cast<IR::Vector<IR::Node> *>(p3)));
    assert(!(*p1 == *static_cast<IR::Node *>(p3)));
    assert(!(*static_cast<IR::Vector<IR::Node> *>(p1) == *p3));
    assert(!(*static_cast<IR::Vector<IR::Node> *>(p1) == *static_cast<IR::Vector<IR::Node> *>(p3)));
    assert(!(*static_cast<IR::Vector<IR::Node> *>(p1) == *static_cast<IR::Node *>(p3)));
    assert(!(*static_cast<IR::Node *>(p1) == *p3));
    assert(!(*static_cast<IR::Node *>(p1) == *static_cast<IR::Vector<IR::Node> *>(p3)));
    assert(!(*static_cast<IR::Node *>(p1) == *static_cast<IR::Node *>(p3)));
}

P4CTEST_REGISTER("test-opeq", test_opeq_main);
