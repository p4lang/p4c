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

#include <functional>

#include "../../lib/enumerator.h"
#include "test.h"

using namespace Util;

namespace Test {
class A {
 public:
    int a;
    explicit A(int a) : a(a) {}
};

class B : public A {
 public:
    explicit B(int b) : A(b) {}
};

class TestEnumerator : public TestBase {
    std::vector<int> vec;

 public:
    TestEnumerator() {
        vec.push_back(1);
        vec.push_back(2);
        vec.push_back(3);
    }

    int run() {
        RUNTEST(testSimple);
        RUNTEST(testLinq);
        RUNTEST(testRange);
        return SUCCESS;
    }

    int testRange() {
        Enumerator<int>* enumerator = Util::Enumerator<int>::createEnumerator(vec);
        int sum = 0;
        for (auto a : *enumerator)
            sum += a;
        ASSERT_EQ(sum, 6);
        return SUCCESS;
    }

    int testLinq() {
        // where
        Enumerator<int>* enumerator = Util::Enumerator<int>::createEnumerator(vec);

        std::function<bool(const int&)> isEven = [](int x) { return x % 2 == 0; };
        Enumerator<int>* even = enumerator->where(isEven);

        bool more = even->moveNext();
        ASSERT_EQ(more, true);

        int elem = even->getCurrent();
        ASSERT_EQ(elem, 2);

        more = even->moveNext();
        ASSERT_EQ(more, false);

        /// map
        {
            enumerator->reset();
            std::function<int(const int&)> increment = [](int x) { return x+1; };
            auto inc = enumerator->map(increment);
            more = inc->moveNext();
            ASSERT_EQ(more, true);
            elem = inc->getCurrent();
            ASSERT_EQ(elem, 2);
            more = inc->moveNext();
            ASSERT_EQ(more, true);
            elem = inc->getCurrent();
            ASSERT_EQ(elem, 3);
            more = inc->moveNext();
            ASSERT_EQ(more, true);
            elem = inc->getCurrent();
            ASSERT_EQ(elem, 4);
            more = inc->moveNext();
            ASSERT_EQ(more, false);

            inc->reset();
            more = inc->moveNext();
            ASSERT_EQ(more, true);
            elem = inc->getCurrent();
            ASSERT_EQ(elem, 2);
            more = inc->moveNext();
            ASSERT_EQ(more, true);
            elem = inc->getCurrent();
            ASSERT_EQ(elem, 3);
            more = inc->moveNext();
            ASSERT_EQ(more, true);
            elem = inc->getCurrent();
            ASSERT_EQ(elem, 4);
            more = inc->moveNext();
            ASSERT_EQ(more, false);
        }

        {
            //// concat
            Enumerator<int>* col1 = Util::Enumerator<int>::createEnumerator(vec);
            Enumerator<int>* col2 = Util::Enumerator<int>::createEnumerator(vec);
            Enumerator<int>* col3 = Util::Enumerator<int>::createEnumerator(vec);
            std::vector<Enumerator<int>*> all;
            all.push_back(col1);
            all.push_back(col2);
            all.push_back(col3);

            Enumerator<Enumerator<int>*>* allEnums =
                    Enumerator<Enumerator<int>*>::createEnumerator(all);
            Enumerator<int>* concat = Enumerator<int>::concatAll(allEnums);
            uint64_t count = concat->count();
            ASSERT_EQ(count, 9);

            concat->reset();
            count = concat->count();
            ASSERT_EQ(count, 9);

            Enumerator<int>* cc1 = Util::Enumerator<int>::createEnumerator(vec);
            Enumerator<int>* cc2 = Util::Enumerator<int>::createEnumerator(vec);
            cc1 = cc1->concat(cc2);
            count = cc1->count();
            ASSERT_EQ(count, 6);
        }

        {
            // as
            std::vector<B*> bs;
            bs.push_back(new B(1));
            bs.push_back(new B(2));
            Enumerator<B*> *benum = Enumerator<B*>::createEnumerator(bs);
            Enumerator<A*> *aenum = benum->as<A*>();
            more = aenum->moveNext();
            ASSERT_EQ(more, true);

            A* a = aenum->getCurrent();
            ASSERT_EQ(a->a, 1);

            more = aenum->moveNext();
            ASSERT_EQ(more, true);
            a = aenum->getCurrent();
            ASSERT_EQ(a->a, 2);

            more = aenum->moveNext();
            ASSERT_EQ(more, false);
        }

        // first & co.
        {
            Enumerator<int>* vi = Util::Enumerator<int>::createEnumerator(vec);
            ASSERT_EQ(vi->next(), 1);
            ASSERT_EQ(vi->nextOrDefault(), 2);
            ASSERT_EQ(vi->nextOrDefault(), 3);
            ASSERT_EQ(vi->nextOrDefault(), 0);

            Enumerator<int>* e = Util::Enumerator<int>::emptyEnumerator();
            try {
                e->next();
                UNREACHABLE();
            }
            catch (std::logic_error&) {}
        }

        {
            std::vector<int> s;
            s.push_back(5);
            Enumerator<int>* e = Util::Enumerator<int>::createEnumerator(s);
            ASSERT_EQ(e->single(), 5);
            try {
                e->single();
                UNREACHABLE();
            }
            catch (std::logic_error&) {}
        }

        return SUCCESS;
    }

    int testSimple() {
        Enumerator<int>* enumerator = Util::Enumerator<int>::createEnumerator(vec);

        bool more = enumerator->moveNext();
        ASSERT_EQ(more, true);
        int elem = enumerator->getCurrent();
        ASSERT_EQ(elem, 1);
        more = enumerator->moveNext();
        ASSERT_EQ(more, true);
        elem = enumerator->getCurrent();
        ASSERT_EQ(elem, 2);
        more = enumerator->moveNext();
        ASSERT_EQ(more, true);
        elem = enumerator->getCurrent();
        ASSERT_EQ(elem, 3);
        more = enumerator->moveNext();
        ASSERT_EQ(more, false);

        try {
            elem = enumerator->getCurrent();
            UNREACHABLE();
        }
        catch (std::logic_error&) {}

        enumerator->reset();
        try {
            elem = enumerator->getCurrent();
            UNREACHABLE();
        }
        catch (std::logic_error&) {}

        more = enumerator->moveNext();
        ASSERT_EQ(more, true);
        elem = enumerator->getCurrent();
        ASSERT_EQ(elem, 1);
        uint64_t size = enumerator->count();
        ASSERT_EQ(size, 2);
        enumerator->reset();
        size = enumerator->count();
        ASSERT_EQ(size, 3);

        return SUCCESS;
    }
};
}  // namespace Test

int main(int, char* []) {
    Test::TestEnumerator test;
    return test.run();
}

