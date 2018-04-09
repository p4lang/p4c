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

#include <exception>
#include <vector>

#include "gtest/gtest.h"
#include "lib/enumerator.h"

namespace Util {

class UtilEnumerator : public ::testing::Test {
 protected:
    class A {
     public:
        int a;
        explicit A(int a) : a(a) {}
    };

    class B : public A {
     public:
        explicit B(int b) : A(b) {}
    };

    std::vector<int> vec{ 1, 2, 3 };
};

TEST_F(UtilEnumerator, Simple) {
    Enumerator<int>* enumerator = Util::Enumerator<int>::createEnumerator(vec);

    bool more = enumerator->moveNext();
    EXPECT_TRUE(more);
    int elem = enumerator->getCurrent();
    EXPECT_EQ(1, elem);
    more = enumerator->moveNext();
    EXPECT_TRUE(more);
    elem = enumerator->getCurrent();
    EXPECT_EQ(2, elem);
    more = enumerator->moveNext();
    EXPECT_TRUE(more);
    elem = enumerator->getCurrent();
    EXPECT_EQ(3, elem);
    more = enumerator->moveNext();
    EXPECT_FALSE(more);

    EXPECT_THROW(enumerator->getCurrent(), std::logic_error);

    enumerator->reset();
    EXPECT_THROW(enumerator->getCurrent(), std::logic_error);

    more = enumerator->moveNext();
    EXPECT_TRUE(more);
    elem = enumerator->getCurrent();
    EXPECT_EQ(1, elem);
    uint64_t size = enumerator->count();
    EXPECT_EQ(2u, size);
    enumerator->reset();
    size = enumerator->count();
    EXPECT_EQ(3u, size);
}

TEST_F(UtilEnumerator, Range) {
    Enumerator<int>* enumerator = Util::Enumerator<int>::createEnumerator(vec);
    int sum = 0;
    for (auto a : *enumerator)
        sum += a;
    EXPECT_EQ(6, sum);
}

TEST_F(UtilEnumerator, Linq) {
    // where
    Enumerator<int>* enumerator = Util::Enumerator<int>::createEnumerator(vec);

    auto isEven = [](int x) { return x % 2 == 0; };
    Enumerator<int>* even = enumerator->where(isEven);

    bool more = even->moveNext();
    EXPECT_TRUE(more);

    int elem = even->getCurrent();
    EXPECT_EQ(2, elem);

    more = even->moveNext();
    EXPECT_FALSE(more);

    /// map
    {
        enumerator->reset();
        std::function<int(const int&)> increment = [](int x) { return x+1; };
        auto inc = enumerator->map(increment);
        more = inc->moveNext();
        EXPECT_TRUE(more);
        elem = inc->getCurrent();
        EXPECT_EQ(2, elem);
        more = inc->moveNext();
        EXPECT_TRUE(more);
        elem = inc->getCurrent();
        EXPECT_EQ(3, elem);
        more = inc->moveNext();
        EXPECT_TRUE(more);
        elem = inc->getCurrent();
        EXPECT_EQ(4, elem);
        more = inc->moveNext();
        EXPECT_FALSE(more);

        inc->reset();
        more = inc->moveNext();
        EXPECT_TRUE(more);
        elem = inc->getCurrent();
        EXPECT_EQ(2, elem);
        more = inc->moveNext();
        EXPECT_TRUE(more);
        elem = inc->getCurrent();
        EXPECT_EQ(3, elem);
        more = inc->moveNext();
        EXPECT_TRUE(more);
        elem = inc->getCurrent();
        EXPECT_EQ(4, elem);
        more = inc->moveNext();
        EXPECT_FALSE(more);
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
        EXPECT_EQ(9u, count);

        concat->reset();
        count = concat->count();
        EXPECT_EQ(9u, count);

        Enumerator<int>* cc1 = Util::Enumerator<int>::createEnumerator(vec);
        Enumerator<int>* cc2 = Util::Enumerator<int>::createEnumerator(vec);
        cc1 = cc1->concat(cc2);
        count = cc1->count();
        EXPECT_EQ(6u, count);
    }

    {
        // as
        std::vector<B*> bs;
        bs.push_back(new B(1));
        bs.push_back(new B(2));
        Enumerator<B*> *benum = Enumerator<B*>::createEnumerator(bs);
        Enumerator<A*> *aenum = benum->as<A*>();
        more = aenum->moveNext();
        EXPECT_TRUE(more);

        A* a = aenum->getCurrent();
        EXPECT_EQ(1, a->a);

        more = aenum->moveNext();
        EXPECT_TRUE(more);
        a = aenum->getCurrent();
        EXPECT_EQ(2, a->a);

        more = aenum->moveNext();
        EXPECT_FALSE(more);
    }

    // first & co.
    {
        Enumerator<int>* vi = Util::Enumerator<int>::createEnumerator(vec);
        EXPECT_EQ(1, vi->next());
        EXPECT_EQ(2, vi->nextOrDefault());
        EXPECT_EQ(3, vi->nextOrDefault());
        EXPECT_EQ(0, vi->nextOrDefault());

        Enumerator<int>* e = Util::Enumerator<int>::emptyEnumerator();
        EXPECT_THROW(e->next(), std::logic_error);
    }

    {
        std::vector<int> s;
        s.push_back(5);
        Enumerator<int>* e = Util::Enumerator<int>::createEnumerator(s);
        EXPECT_EQ(5, e->single());
        EXPECT_THROW(e->single(), std::logic_error);
    }
}

}  // namespace Util
