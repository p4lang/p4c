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

#include "lib/enumerator.h"

#include <gtest/gtest.h>

#include <exception>
#include <vector>

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

    std::vector<int> vec{1, 2, 3};
};

TEST_F(UtilEnumerator, Simple) {
    Enumerator<int> *enumerator = Util::enumerate(vec);

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
    Enumerator<int> *enumerator = Util::enumerate(vec);
    int sum = 0;
    for (auto a : *enumerator) sum += a;
    EXPECT_EQ(6, sum);
}

TEST_F(UtilEnumerator, Linq) {
    // where
    Enumerator<int> *enumerator = Util::enumerate(vec);

    auto isEven = [](int x) { return x % 2 == 0; };
    Enumerator<int> *even = enumerator->where(isEven);

    bool more = even->moveNext();
    EXPECT_TRUE(more);

    int elem = even->getCurrent();
    EXPECT_EQ(2, elem);

    more = even->moveNext();
    EXPECT_FALSE(more);

    /// map
    {
        enumerator->reset();
        std::function<int(const int &)> increment = [](int x) { return x + 1; };
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
        Enumerator<int> *col1 = Util::enumerate(vec);
        Enumerator<int> *col2 = Util::enumerate(vec);
        Enumerator<int> *col3 = Util::enumerate(vec);
        std::vector<Enumerator<int> *> all{col1, col2, col3};

        Enumerator<Enumerator<int> *> *allEnums = Util::enumerate(all);
        Enumerator<int> *concat = Enumerator<int>::concatAll(allEnums);
        uint64_t count = concat->count();
        EXPECT_EQ(9u, count);

        concat->reset();
        count = concat->count();
        EXPECT_EQ(9u, count);

        concat = Util::concat({col1, col2, col3});
        concat->reset();
        count = concat->count();
        EXPECT_EQ(9u, count);

        concat = Util::concat(col1, col2, col3);
        concat->reset();
        count = concat->count();
        EXPECT_EQ(9u, count);

        Enumerator<int> *cc1 = Util::enumerate(vec);
        Enumerator<int> *cc2 = Util::enumerate(vec);
        cc1 = cc1->concat(cc2);
        count = cc1->count();
        EXPECT_EQ(6u, count);

        // cc1 is a ConcatEnumerator. Therefore its ->concat returns the object
        // itself
        cc1->reset();
        auto *cc3 = cc1->concat(col3);
        EXPECT_EQ(cc1, cc3);
        cc1->reset();
        count = cc1->count();
        EXPECT_EQ(9u, count);
    }

    {
        // as
        std::vector<B *> bs;
        bs.push_back(new B(1));
        bs.push_back(new B(2));
        Enumerator<B *> *benum = Util::enumerate(bs);
        Enumerator<A *> *aenum = benum->as<A *>();
        more = aenum->moveNext();
        EXPECT_TRUE(more);

        A *a = aenum->getCurrent();
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
        Enumerator<int> *vi = Util::enumerate(vec);
        EXPECT_EQ(1, vi->next());
        EXPECT_EQ(2, vi->nextOrDefault());
        EXPECT_EQ(3, vi->nextOrDefault());
        EXPECT_EQ(0, vi->nextOrDefault());

        Enumerator<int> *e = Util::Enumerator<int>::emptyEnumerator();
        EXPECT_THROW(e->next(), std::logic_error);
    }

    {
        std::vector<int> s;
        s.push_back(5);
        Enumerator<int> *e = Util::enumerate(s);
        EXPECT_EQ(5, e->single());
        EXPECT_THROW(e->single(), std::logic_error);
    }
}

}  // namespace Util
