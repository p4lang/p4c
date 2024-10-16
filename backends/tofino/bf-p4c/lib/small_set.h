/**
 * Copyright 2013-2024 Intel Corporation.
 *
 * This software and the related documents are Intel copyrighted materials, and your use of them
 * is governed by the express license under which they were provided to you ("License"). Unless
 * the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
 * or transmit this software or the related documents without Intel's prior written permission.
 *
 * This software and the related documents are provided as is, with no express or implied
 * warranties, other than those that are expressly stated in the License.
 */

#ifndef EXTENSIONS_BF_P4C_LIB_SMALL_SET_H_
#define EXTENSIONS_BF_P4C_LIB_SMALL_SET_H_

#include <algorithm>
#include <variant>
#include <vector>

#include "lib/ordered_set.h"

using namespace P4;

template <typename T>
class ConstSmallSetIterator {
    using set_iterator = typename ordered_set<T>::const_iterator;
    using vector_iterator = typename std::vector<T>::const_iterator;

    std::variant<set_iterator, vector_iterator> self;

 public:
    using difference_type = ptrdiff_t;
    using value_type = T;
    using pointer = const T*;
    using reference = const T&;
    using iterator_category = std::bidirectional_iterator_tag;

    explicit ConstSmallSetIterator(set_iterator it) noexcept : self(it) {}
    explicit ConstSmallSetIterator(vector_iterator it) noexcept : self(it) {}

    bool operator==(const ConstSmallSetIterator& rhs) const noexcept { return self == rhs.self; }

    bool operator!=(const ConstSmallSetIterator& rhs) const noexcept { return !(*this == rhs); }

    ConstSmallSetIterator& operator++() noexcept {
        std::visit([](auto&& it) { ++it; }, self);
        return *this;
    }
    ConstSmallSetIterator operator++(int) noexcept {
        return std::visit(
            [](auto&& it) -> ConstSmallSetIterator { return ConstSmallSetIterator(it++); }, self);
    }

    ConstSmallSetIterator& operator--() noexcept {
        std::visit([](auto&& it) { --it; }, self);
        return *this;
    }
    ConstSmallSetIterator operator--(int) noexcept {
        return std::visit(
            [](auto&& it) -> ConstSmallSetIterator { return ConstSmallSetIterator(it--); }, self);
    }

    reference operator*() const noexcept {
        return std::visit([](auto&& it) -> reference { return *it; }, self);
    }
    pointer operator->() const noexcept { return &this->operator*(); }

    template <typename U, std::size_t N>
    friend class SmallSet;
};

/**
 * @brief SmallSet<T> is a set type optimized for a small amount of items kept in insertion-order.
 * Items are stored in a std::vector<T> initially, which offers good iteration performance. Once the
 * amount of items surpasses the threshold, internal representation is switched to ordered_set<T> to
 * keep efficient lookup and insertion performance.
 *
 * @tparam T type of elements
 * @tparam N maximum number of items to keep in the vector
 */
template <typename T, std::size_t N = 64>
class SmallSet {
    using VectorType = std::vector<T>;
    using SetType = ordered_set<T>;

    std::variant<VectorType, SetType> data;

    template <typename Container>
    constexpr static bool is_small(Container&& container) noexcept {
        return std::is_same_v<std::decay_t<decltype(container)>, VectorType>;
    }

    typename VectorType::const_iterator vfind(const VectorType& vector, const T& key) const {
        return std::find(vector.begin(), vector.end(), key);
    }

 public:
    using const_iterator = ConstSmallSetIterator<T>;
    using key_type = T;

    SmallSet() : data(std::in_place_type_t<VectorType>()) {}
    SmallSet(std::initializer_list<T> init) {
        if (init.size() <= N) {
            data = VectorType(init);
        } else {
            data = SetType(init);
        }
    }

    /**
     * @return const_iterator to the first element
     */
    const_iterator begin() const noexcept {
        return std::visit(
            [](auto&& container) -> const_iterator { return const_iterator(container.begin()); },
            data);
    }

    /**
     * @return const_iterator - the past-the-end iterator
     */
    const_iterator end() const noexcept {
        return std::visit(
            [](auto&& container) -> const_iterator { return const_iterator(container.end()); },
            data);
    }

    /**
     * @return std::size_t - number of values in the set
     */
    std::size_t size() const noexcept {
        return std::visit([](auto&& container) -> std::size_t { return container.size(); }, data);
    }

    /**
     * @brief Remove all values from the set
     *
     */
    void clear() noexcept {
        std::visit([](auto&& container) { container.clear(); }, data);
    }

    [[nodiscard]] bool empty() const noexcept { return size() == 0ul; }

    /**
     * @brief Find given key in the set. Note that this method compares keys with operator== as
     * opposed to operator< in most other set types.
     *
     * @param key the value to find
     * @return const_iterator to found item, or end() if not found
     */
    const_iterator find(const T& key) const {
        return std::visit(
            [&](auto&& container) -> const_iterator {
                if constexpr (is_small(container)) {
                    return const_iterator(vfind(container, key));
                } else {
                    return const_iterator(container.find(key));
                }
            },
            data);
    }

    /**
     * @brief Check if there is an element in the set equivalent with @p key (using operator==).
     */
    bool contains(const T& key) const { return find(key) != end(); }

    /**
     * @brief inserts the value if it's not already present in the set
     *
     * @param value the value to insert
     * @return std::pair<const_iterator, bool> - If the value was inserted, returns iterator to it and
     * true. If the value was not inserted, returns iterator to the element that prevented insertion
     * and false.
     */
    template <typename Value>
    std::pair<const_iterator, bool> insert(Value&& value) {
        return std::visit(
            [this, &value](auto&& container) -> std::pair<const_iterator, bool> {
                if constexpr (is_small(container)) {
                    const auto it = vfind(container, value);
                    if (it == container.end()) {
                        if (container.size() < N) {
                            container.push_back(std::forward<Value>(value));
                            return {const_iterator(std::prev(container.end())), true};
                        }

                        SetType set;
                        std::move(container.begin(), container.end(), std::back_inserter(set));

                        data = std::move(set);
                        auto [set_it, inserted] =
                            std::get<SetType>(data).insert(std::forward<Value>(value));
                        return {const_iterator(set_it), inserted};
                    }
                    return {const_iterator(it), false};
                } else {
                    auto [set_it, inserted] = container.insert(std::forward<Value>(value));
                    return {const_iterator(set_it), inserted};
                }
            },
            data);
    }
    /**
     * @brief inserts values from range [first, last), if they are not already present in the set
     *
     * @tparam InputIt type of input iterator
     * @param first iterator of the beginning of range of values to insert
     * @param last iterator of the end of range of values to insert
     */
    template <typename InputIt>
    void insert(InputIt first, InputIt last) {
        std::for_each(first, last, [this](const T& key) { insert(key); });
    }

    /**
     * @param key value to return the count of
     * @return std::size_t: 1 if the value is present in the set, 0 if not
     */
    std::size_t count(const T& key) const { return std::size_t(contains(key)); }

    /**
     * @param key value to remove from the set
     * @return std::size_t: 1 if the value was removed, 0 if not
     */
    std::size_t erase(const T& key) {
        return std::visit(
            [this, &key](auto&& container) -> std::size_t {
                if constexpr (is_small(container)) {
                    const auto it = vfind(container, key);
                    if (it != container.end()) {
                        container.erase(it);
                        return 1;
                    }
                    return 0;
                } else {
                    return container.erase(key);
                }
            },
            data);
    }

    /**
     * @brief An efficient way to remove another set of values from the set, meant to replace loops
     * of erase(key).
     *
     * @tparam Set any container of values of type T that provides a method count(T)
     * @param to_remove set of values to remove
     */
    template <typename Set>
    void erase_set(const Set& to_remove) {
        std::visit(
            [&](auto&& container) {
                if constexpr (is_small(container)) {
                    container.erase(
                        std::remove_if(container.begin(), container.end(),
                                       [&](const T& key) { return to_remove.count(key); }),
                        container.end());
                } else {
                    for (const auto& key : to_remove) {
                        container.erase(key);
                    }
                }
            },
            data);
    }

    bool operator==(const SmallSet& rhs) const { return data == rhs.data; }

    /**
     * @brief Reserve space for @p n total items in the underlaying vector. If the data is in an
     * ordered_set, this method does nothing.
     *
     * @param n total number of items to reserve space for
     */
    void reserve(const std::size_t n) {
        std::visit(
            [=](auto&& container) {
                if constexpr (is_small(container)) {
                    container.reserve(n);
                }
            },
            data);
    }
};

#endif  // EXTENSIONS_BF_P4C_LIB_SMALL_SET_H_
