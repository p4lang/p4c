// SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
// Copyright 2013-present Barefoot Networks, Inc.
//
// SPDX-License-Identifier: Apache-2.0

#include "lib/enumerator.h"

#include <gtest/gtest.h>

#include <exception>
#include <vector>

namespace P4::Util {

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
    EnumeratorPtr<int> enumerator = Util::enumerate(vec);

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
    EnumeratorPtr<int> enumerator = Util::enumerate(vec);
    int sum = 0;
    for (auto a : *enumerator) sum += a;
    EXPECT_EQ(6, sum);
}

TEST_F(UtilEnumerator, Linq) {
    // where
    EnumeratorPtr<int> enumerator = Util::enumerate(vec);

    auto isEven = [](int x) { return x % 2 == 0; };
    EnumeratorPtr<int> even = enumerator->where(isEven);

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
        EnumeratorPtr<int> col1 = Util::enumerate(vec);
        EnumeratorPtr<int> col2 = Util::enumerate(vec);
        EnumeratorPtr<int> col3 = Util::enumerate(vec);
        std::vector<EnumeratorPtr<int>> all{col1, col2, col3};

        EnumeratorPtr<EnumeratorPtr<int>> allEnums = Util::enumerate(all);
        EnumeratorPtr<int> concat = Enumerator<int>::concatAll(allEnums);
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

        EnumeratorPtr<int> cc1 = Util::enumerate(vec);
        EnumeratorPtr<int> cc2 = Util::enumerate(vec);
        cc1 = cc1->concat(cc2);
        count = cc1->count();
        EXPECT_EQ(6u, count);

        // cc1 is a ConcatEnumerator. Therefore its ->concat returns the object
        // itself
        cc1->reset();
        auto cc3 = cc1->concat(col3);
        EXPECT_EQ(cc1, cc3);
        cc1->reset();
        count = cc1->count();
        EXPECT_EQ(9u, count);
    }

    {
        // as
        std::vector<B *> bs;
        B first(1), second(2);
        bs.push_back(&first);
        bs.push_back(&second);
        EnumeratorPtr<B *> benum = Util::enumerate(bs);
        EnumeratorPtr<A *> aenum = benum->as<A *>();
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
        EnumeratorPtr<int> vi = Util::enumerate(vec);
        EXPECT_EQ(1, vi->next());
        EXPECT_EQ(2, vi->nextOrDefault());
        EXPECT_EQ(3, vi->nextOrDefault());
        EXPECT_EQ(0, vi->nextOrDefault());

        EnumeratorPtr<int> e = Util::Enumerator<int>::emptyEnumerator();
        EXPECT_THROW(e->next(), std::logic_error);
    }

    {
        std::vector<int> s;
        s.push_back(5);
        EnumeratorPtr<int> e = Util::enumerate(s);
        EXPECT_EQ(5, e->single());
        EXPECT_THROW(e->single(), std::logic_error);
    }
}

TEST_F(UtilEnumerator, ChainOwnsAndReleasesInputs) {
    auto input = Util::enumerate(vec);
    std::weak_ptr<Enumerator<int>> lifetime = input;
    auto chain =
        input->where([](int value) { return value > 1; })->map([](int value) { return value * 2; });
    input.reset();
    EXPECT_FALSE(lifetime.expired());
    EXPECT_EQ(chain->toVector(), (std::vector<int>{4, 6}));
    chain.reset();
    EXPECT_TRUE(lifetime.expired());
}

TEST_F(UtilEnumerator, TemporaryRangeRetainsCapturedState) {
    auto state = std::make_shared<int>(2);
    std::weak_ptr<int> lifetime = state;
    int sum = 0;
    for (auto value : Util::enumerate(vec)->map(
             [state = std::move(state)](int value) { return value * *state; })) {
        EXPECT_FALSE(lifetime.expired());
        sum += value;
    }
    EXPECT_EQ(sum, 12);
    EXPECT_TRUE(lifetime.expired());
}

TEST_F(UtilEnumerator, ConcatenationReleasesInputs) {
    auto input = Util::enumerate(vec);
    std::weak_ptr<Enumerator<int>> lifetime = input;
    auto chain = input->concat(Util::enumerate(vec));
    input.reset();
    EXPECT_EQ(chain->count(), 6U);
    EXPECT_FALSE(lifetime.expired());
    chain.reset();
    EXPECT_TRUE(lifetime.expired());
}

}  // namespace P4::Util
