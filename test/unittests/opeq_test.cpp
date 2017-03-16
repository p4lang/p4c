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
#include "lib/exceptions.h"

#define CHECK(E) ((E) ? 0 : ((std::cerr <<  #E " failed" << std::endl), ++errcnt))

int main() {
    int errcnt = 0;
    auto *t = IR::Type::Bits::get(16);
    IR::Constant *a = new IR::Constant(t, 10);
    IR::Constant *b = new IR::Constant(t, 10);
    IR::Constant *c = new IR::Constant(t, 20);

    CHECK(*a == *b);
    CHECK(*a == *static_cast<IR::Expression *>(b));
    CHECK(*a == *static_cast<IR::Node *>(b));
    CHECK(*static_cast<IR::Expression *>(a) == *b);
    CHECK(*static_cast<IR::Expression *>(a) == *static_cast<IR::Expression *>(b));
    CHECK(*static_cast<IR::Expression *>(a) == *static_cast<IR::Node *>(b));
    CHECK(*static_cast<IR::Node *>(a) == *b);
    CHECK(*static_cast<IR::Node *>(a) == *static_cast<IR::Expression *>(b));
    CHECK(*static_cast<IR::Node *>(a) == *static_cast<IR::Node *>(b));

    CHECK(!(*a == *c));
    CHECK(!(*a == *static_cast<IR::Expression *>(c)));
    CHECK(!(*a == *static_cast<IR::Node *>(c)));
    CHECK(!(*static_cast<IR::Expression *>(a) == *c));
    CHECK(!(*static_cast<IR::Expression *>(a) == *static_cast<IR::Expression *>(c)));
    CHECK(!(*static_cast<IR::Expression *>(a) == *static_cast<IR::Node *>(c)));
    CHECK(!(*static_cast<IR::Node *>(a) == *c));
    CHECK(!(*static_cast<IR::Node *>(a) == *static_cast<IR::Expression *>(c)));
    CHECK(!(*static_cast<IR::Node *>(a) == *static_cast<IR::Node *>(c)));

    auto *p1 = new IR::IndexedVector<IR::Node>(a);
    auto *p2 = p1->clone();
    auto *p3 = new IR::IndexedVector<IR::Node>(c);

    CHECK(*p1 == *p2);
    CHECK(*p1 == *static_cast<IR::Vector<IR::Node> *>(p2));
    CHECK(*p1 == *static_cast<IR::Node *>(p2));
    CHECK(*static_cast<IR::Vector<IR::Node> *>(p1) == *p2);
    CHECK(*static_cast<IR::Vector<IR::Node> *>(p1) == *static_cast<IR::Vector<IR::Node> *>(p2));
    CHECK(*static_cast<IR::Vector<IR::Node> *>(p1) == *static_cast<IR::Node *>(p2));
    CHECK(*static_cast<IR::Node *>(p1) == *p2);
    CHECK(*static_cast<IR::Node *>(p1) == *static_cast<IR::Vector<IR::Node> *>(p2));
    CHECK(*static_cast<IR::Node *>(p1) == *static_cast<IR::Node *>(p2));

    CHECK(!(*p1 == *p3));
    CHECK(!(*p1 == *static_cast<IR::Vector<IR::Node> *>(p3)));
    CHECK(!(*p1 == *static_cast<IR::Node *>(p3)));
    CHECK(!(*static_cast<IR::Vector<IR::Node> *>(p1) == *p3));
    CHECK(!(*static_cast<IR::Vector<IR::Node> *>(p1) == *static_cast<IR::Vector<IR::Node> *>(p3)));
    CHECK(!(*static_cast<IR::Vector<IR::Node> *>(p1) == *static_cast<IR::Node *>(p3)));
    CHECK(!(*static_cast<IR::Node *>(p1) == *p3));
    CHECK(!(*static_cast<IR::Node *>(p1) == *static_cast<IR::Vector<IR::Node> *>(p3)));
    CHECK(!(*static_cast<IR::Node *>(p1) == *static_cast<IR::Node *>(p3)));

    return errcnt > 0;
}
