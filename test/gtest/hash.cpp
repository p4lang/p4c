/*
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

#include "lib/hash.h"

#include <gtest/gtest.h>

#include <unordered_map>
#include <unordered_set>

#include "lib/cstring.h"

namespace Test {

TEST(Hash, SimpleHashCombine) {
    constexpr uint64_t upper = UINT64_C(0xDEADBEEF12345678);
    constexpr uint64_t lower = UINT64_C(0xBADF00D910111213);
    EXPECT_NE(Util::hash_combine(upper, lower), Util::hash_combine(lower, upper));
}

TEST(Hash, XXHash) {
    const char *s1 = "hello, world!";
    const uint64_t s1_hash = UINT64_C(4021878480306939596);
    EXPECT_EQ(Util::hash(s1, strlen(s1)), s1_hash);

    cstring s2(s1);
    EXPECT_EQ(Util::hash(s2.c_str(), s2.size()), s1_hash);

    std::string s3(s1);
    EXPECT_EQ(Util::hash(s3), s1_hash);
}

TEST(Hash, Hasher) {
    std::unordered_map<int32_t, int32_t, Util::Hasher<int32_t>> m;
    m.emplace(123, 42);
    EXPECT_EQ(m.at(123), 42);
}

TEST(Hash, IntegerTypes) {
    std::unordered_set<size_t> hashes;
    Util::Hash hasher;
    hashes.insert(hasher((char)1));
    hashes.insert(hasher((signed char)2));
    hashes.insert(hasher((unsigned char)3));
    hashes.insert(hasher((short)4));
    hashes.insert(hasher((signed short)5));
    hashes.insert(hasher((unsigned short)6));
    hashes.insert(hasher((int)7));
    hashes.insert(hasher((signed int)8));
    hashes.insert(hasher((unsigned int)9));
    hashes.insert(hasher((long)10));
    hashes.insert(hasher((signed long)11));
    hashes.insert(hasher((unsigned long)12));
    hashes.insert(hasher((long long)13));
    hashes.insert(hasher((signed long long)14));
    hashes.insert(hasher((unsigned long long)15));
    hashes.insert(hasher((int8_t)16));
    hashes.insert(hasher((uint8_t)17));
    hashes.insert(hasher((int16_t)18));
    hashes.insert(hasher((uint16_t)19));
    hashes.insert(hasher((int32_t)20));
    hashes.insert(hasher((uint32_t)21));
    hashes.insert(hasher((int64_t)22));
    hashes.insert(hasher((uint64_t)23));
    hashes.insert(hasher((size_t)24));

    size_t hashesSize = 24;
    EXPECT_EQ(hashesSize, hashes.size());
}

TEST(Hash, IntegerConversion) {
    Util::Hasher<uint64_t> h;
    uint64_t k = 10;
    EXPECT_EQ(h(k), h(10));
}

TEST(Hash, FloatTypes) {
    Util::Hash hasher;

    EXPECT_EQ(hasher(0.0f), hasher(-0.0f));
    EXPECT_EQ(hasher(0.0), hasher(-0.0));

    std::unordered_set<size_t> hashes;
    hashes.insert(hasher(0.0f));
    hashes.insert(hasher(0.1f));
    hashes.insert(hasher(0.2));
    hashes.insert(hasher(0.2f));
    hashes.insert(hasher(-0.3));
    hashes.insert(hasher(-0.3f));

    EXPECT_EQ(6, hashes.size());
}

class TestHasher {
 public:
    size_t operator()(const std::pair<unsigned, unsigned> &val) const {
        return val.first + val.second;
    }
};

template <typename T, typename... Ts>
size_t hash_combine_test(const T &t, const Ts &...ts) {
    return Util::hash_combine_generic(TestHasher{}, t, ts...);
}

TEST(Hash, Pair) {
    auto a = std::make_pair(1, 2);
    auto b = std::make_pair(3, 4);
    auto c = std::make_pair(1, 2);
    auto d = std::make_pair(2, 1);
    EXPECT_EQ(Util::hash_combine(a), Util::hash_combine(c));
    EXPECT_NE(Util::hash_combine(b), Util::hash_combine(c));
    EXPECT_NE(Util::hash_combine(d), Util::hash_combine(c));

    EXPECT_EQ(Util::hash_combine(a, b), Util::hash_combine(c, b));
    EXPECT_NE(Util::hash_combine(a, b), Util::hash_combine(b, a));

    EXPECT_EQ(hash_combine_test(a), hash_combine_test(c));
    EXPECT_NE(hash_combine_test(b), hash_combine_test(c));
    EXPECT_EQ(hash_combine_test(d), hash_combine_test(c));
    EXPECT_EQ(hash_combine_test(a, b), hash_combine_test(c, b));
    EXPECT_NE(hash_combine_test(a, b), hash_combine_test(b, a));
    EXPECT_EQ(hash_combine_test(a, b), hash_combine_test(d, b));
}

TEST(Hash, Bool) {
    const auto hash = Util::Hash();
    EXPECT_NE(hash(true), hash(false));
}

TEST(Hash, HashCombine) { EXPECT_NE(Util::hash_combine(1, 2), Util::hash_combine(2, 1)); }

TEST(Hash, HashCombine10Bool) {
    const auto hash = Util::Hash();
    std::set<size_t> values;
    for (bool b1 : {false, true}) {
        for (bool b2 : {false, true}) {
            for (bool b3 : {false, true}) {
                for (bool b4 : {false, true}) {
                    for (bool b5 : {false, true}) {
                        for (bool b6 : {false, true}) {
                            for (bool b7 : {false, true}) {
                                for (bool b8 : {false, true}) {
                                    for (bool b9 : {false, true}) {
                                        for (bool b10 : {false, true}) {
                                            values.insert(
                                                hash(b1, b2, b3, b4, b5, b6, b7, b8, b9, b10));
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    EXPECT_EQ(values.size(), 1U << 10);  // All hash values must be distinct
}

TEST(Hash, HashCombine10Int) {
    const auto hash = Util::Hash();
    std::set<size_t> values;
    for (int i1 : {1, 2, 3}) {
        for (int i2 : {1, 2, 3}) {
            for (int i3 : {1, 2, 3}) {
                for (int i4 : {1, 2, 3}) {
                    for (int i5 : {1, 2, 3}) {
                        for (int i6 : {1, 2, 3}) {
                            for (int i7 : {1, 2, 3}) {
                                for (int i8 : {1, 2, 3}) {
                                    for (int i9 : {1, 2, 3}) {
                                        for (int i10 : {1, 2, 3}) {
                                            values.insert(
                                                hash(i1, i2, i3, i4, i5, i6, i7, i8, i9, i10));
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    EXPECT_EQ(values.size(), 59049);  // All hash values must be distinct
}

TEST(Hash, StdTuple) {
    typedef std::tuple<int64_t, std::string, int32_t> triple;
    triple t(42, "foo", 1);

    std::unordered_map<triple, std::string> m;
    m[t] = "bar";
    EXPECT_EQ("bar", m[t]);
}

TEST(Hash, EmptyStdTuple) {
    std::unordered_map<std::tuple<>, std::string, Util::Hash> m;
    m[{}] = "foo";
    EXPECT_EQ(m[{}], "foo");

    Util::Hasher<std::tuple<>> h;
    EXPECT_EQ(h({}), 0);
}

TEST(Hash, PairHash) {
    typedef std::pair<int64_t, int32_t> pair2;
    pair2 p(42, 1);

    std::unordered_map<pair2, std::string, Util::Hash> m;
    m[p] = "foo";
    EXPECT_EQ("foo", m[p]);
}

TEST(Hash, TupleHash) {
    typedef std::tuple<int64_t, int32_t, int32_t> triple;
    typedef std::tuple<int64_t, std::string, int32_t> triple2;
    triple t(42, 1, 2);

    std::unordered_map<triple, std::string, Util::Hash> m;
    m[t] = "bar";
    EXPECT_EQ("bar", m[t]);

    triple2 t1(42, "foo", 1);
    triple2 t2(9, "bar", 3);
    triple2 t3(42, "foo", 3);

    EXPECT_NE(std::hash<triple2>()(t1), std::hash<triple2>()(t2));
    EXPECT_NE(std::hash<triple2>()(t1), std::hash<triple2>()(t3));
}

namespace {
template <class T>
size_t hashVector(const std::vector<T> &v) {
    return Util::hash_range(v.begin(), v.end());
}
}  // namespace

TEST(Hash, HashRange) {
    EXPECT_EQ(hashVector<int32_t>({1, 2}), hashVector<int16_t>({1, 2}));
    EXPECT_NE(hashVector<int>({2, 1}), hashVector<int>({1, 2}));
    EXPECT_EQ(hashVector<int>({}), hashVector<float>({}));
}

TEST(Hash, Strings) {
    const char *a1 = "1234567", *b1 = "34957834";
    Util::Hash h;
    EXPECT_NE(h(a1), h(b1));

    EXPECT_EQ(h(std::string{a1}), h(std::string_view{a1}));
    EXPECT_EQ(h(std::string{b1}), h(std::string_view{b1}));
}

namespace {
void deletePointer(const std::unique_ptr<std::string> &) {}
void deletePointer(const std::shared_ptr<std::string> &) {}
void deletePointer(std::string *pointer) { delete pointer; }

template <template <typename...> typename Pointer>
void pointerTestWithHash() {
    std::unordered_set<Pointer<std::string>, Util::Hash> set;

    for (auto i = 0; i < 1000; ++i) set.emplace(new std::string{std::to_string(i)});

    for (auto &pointer : set) {
        EXPECT_TRUE(set.find(pointer) != set.end());
        deletePointer(pointer);
    }
}

template <typename T>
using Pointer = T *;
}  // namespace

TEST(Hash, UniquePtr) { pointerTestWithHash<std::unique_ptr>(); }

TEST(Hash, SharedPtr) { pointerTestWithHash<std::shared_ptr>(); }

TEST(Hash, Pointer) { pointerTestWithHash<Pointer>(); }

}  // namespace Test
