/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* -*-c++-*-
   C#-like enumerator interface */

#ifndef LIB_ENUMERATOR_H_
#define LIB_ENUMERATOR_H_

#include <cstdint>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

#include "config.h"
#include "iterator_range.h"

namespace P4::Util {
enum class EnumeratorState { NotStarted, Valid, PastEnd };

template <typename T>
class Enumerator;

template <typename T>
using EnumeratorPtr = std::shared_ptr<Enumerator<T>>;

/// This class provides support for C++-style range for loops
/// EnumeratorPtr<T> e;
/// for (auto a : e) ...
// FIXME: It is not a proper iterator (see reference type above) and should be removed
// in favor of more standard approach. Note that Enumerator<T>::getCurrent() always
// returns element by value, so more or less suitable only for copyable types that are cheap
// to copy.
template <typename T>
class EnumeratorHandle {
 private:
    Enumerator<T> *enumerator = nullptr;  // when nullptr it represents end()
    explicit EnumeratorHandle(Enumerator<T> *enumerator) : enumerator(enumerator) {}
    friend class Enumerator<T>;

 public:
    using iterator_category = std::input_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = T;
    using reference = T;
    using pointer = void;

    reference operator*() const;
    const EnumeratorHandle<T> &operator++();
    bool operator==(const EnumeratorHandle<T> &other) const;
    bool operator!=(const EnumeratorHandle<T> &other) const;
};

/// Type-erased Enumerator interface
template <class T>
class Enumerator : public std::enable_shared_from_this<Enumerator<T>> {
 protected:
    EnumeratorState state = EnumeratorState::NotStarted;

    // This is a weird oddity of C++: this class is a friend of itself with different templates
    template <class S>
    friend class Enumerator;

    static std::vector<T> emptyVector;
    template <typename S>
    friend class EnumeratorHandle;

 public:
    using value_type = T;

    Enumerator() { this->reset(); }

    virtual ~Enumerator() = default;

    /// Move to next element in the collection;
    /// return false if the next element does not exist
    virtual bool moveNext() = 0;
    /// Get current element in the collection
    virtual T getCurrent() const = 0;
    /// Move back to the beginning of the collection
    virtual void reset() { this->state = EnumeratorState::NotStarted; }

    EnumeratorHandle<T> begin() {
        this->moveNext();
        return EnumeratorHandle<T>(this);
    }
    EnumeratorHandle<T> end() { return EnumeratorHandle<T>(nullptr); }

    const char *stateName() const {
        switch (this->state) {
            case EnumeratorState::NotStarted:
                return "NotStarted";
            case EnumeratorState::Valid:
                return "Valid";
            case EnumeratorState::PastEnd:
                return "PastEnd";
        }
        throw std::logic_error("Unexpected state " + std::to_string(static_cast<int>(this->state)));
    }

    ////////////////// factories
    template <typename Container>
    [[deprecated(
        "Use Util::enumerate() instead")]] static EnumeratorPtr<typename Container::value_type>
    createEnumerator(const Container &data);
    static EnumeratorPtr<T> emptyEnumerator();  // empty data
    template <typename Iter>
    [[deprecated("Use Util::enumerate() instead")]] static EnumeratorPtr<typename Iter::value_type>
    createEnumerator(Iter begin, Iter end);
    template <typename Iter>
    [[deprecated("Use Util::enumerate() instead")]] static EnumeratorPtr<typename Iter::value_type>
    createEnumerator(iterator_range<Iter> range);

    /// Return an enumerator returning all elements that pass the filter
    template <typename Filter>
    EnumeratorPtr<T> where(Filter filter);
    /// Apply specified function to all elements of this enumerator
    template <typename Mapper>
    EnumeratorPtr<std::invoke_result_t<Mapper, T>> map(Mapper map);
    /// Cast to an enumerator of S objects
    template <typename S>
    EnumeratorPtr<S> as();
    /// Append all elements of other after all elements of this
    virtual EnumeratorPtr<T> concat(EnumeratorPtr<T> other);
    /// Concatenate all these collections into a single one
    static EnumeratorPtr<T> concatAll(EnumeratorPtr<EnumeratorPtr<T>> inputs);

    std::vector<T> toVector() {
        std::vector<T> result;
        while (moveNext()) result.push_back(getCurrent());
        return result;
    }

    /// Enumerate all elements and return the count
    uint64_t count() {
        uint64_t found = 0;
        while (this->moveNext()) found++;
        return found;
    }

    /// True if the enumerator has at least one element
    bool any() { return this->moveNext(); }

    /// The only next element; throws if the enumerator does not have exactly 1 element
    T single() {
        bool next = moveNext();
        if (!next) throw std::logic_error("There is no element for `single()'");
        T result = getCurrent();
        next = moveNext();
        if (next) throw std::logic_error("There are multiple elements when calling `single()'");
        return result;
    }

    /// The only next element or default value if none exists; throws if the
    /// enumerator does not have exactly 0 or 1 element
    T singleOrDefault() {
        bool next = moveNext();
        if (!next) return T{};
        T result = getCurrent();
        next = moveNext();
        if (next) throw std::logic_error("There are multiple elements when calling `single()'");
        return result;
    }

    /// Next element, or the default value if none exists
    T nextOrDefault() {
        bool next = moveNext();
        if (!next) return T{};
        return getCurrent();
    }

    /// Next element; throws if there are no elements
    T next() {
        bool next = moveNext();
        if (!next) throw std::logic_error("There is no element for `next()'");
        return getCurrent();
    }
};

////////////////////////// implementation ///////////////////////////////////////////
// the implementation must be in the header file due to the templates

/// We have to be careful never to invoke getCurrent more than once on each
/// element of the input iterators, since it could have side-effects

/// A generic iterator returning elements of type T.
template <typename Iter>
class IteratorEnumerator : public Enumerator<typename std::iterator_traits<Iter>::value_type> {
 protected:
    Iter begin;
    Iter end;
    Iter current;
    const char *name;
    friend class Enumerator<typename std::iterator_traits<Iter>::value_type>;

 public:
    IteratorEnumerator(Iter begin, Iter end, const char *name)
        : Enumerator<typename std::iterator_traits<Iter>::value_type>(),
          begin(begin),
          end(end),
          current(begin),
          name(name) {}

    [[nodiscard]] std::string toString() const {
        return std::string(this->name) + ":" + this->stateName();
    }

    bool moveNext() override {
        switch (this->state) {
            case EnumeratorState::NotStarted:
                this->current = this->begin;
                if (this->current == this->end) {
                    this->state = EnumeratorState::PastEnd;
                    return false;
                } else {
                    this->state = EnumeratorState::Valid;
                }
                return true;
            case EnumeratorState::PastEnd:
                return false;
            case EnumeratorState::Valid:
                ++this->current;
                if (this->current == this->end) {
                    this->state = EnumeratorState::PastEnd;
                    return false;
                }
                return true;
        }

        throw std::runtime_error("Unexpected enumerator state");
    }

    typename std::iterator_traits<Iter>::value_type getCurrent() const override {
        switch (this->state) {
            case EnumeratorState::NotStarted:
                throw std::logic_error("You cannot call 'getCurrent' before 'moveNext'");
            case EnumeratorState::PastEnd:
                throw std::logic_error("You cannot call 'getCurrent' past the collection end");
            case EnumeratorState::Valid:
                return *this->current;
        }
        throw std::runtime_error("Unexpected enumerator state");
    }
};

template <typename Iter>
IteratorEnumerator(Iter begin, Iter end, const char *name) -> IteratorEnumerator<Iter>;

/////////////////////////////////////////////////////////////////////

template <typename T>
class SingleEnumerator : public Enumerator<T> {
    T value;

 public:
    explicit SingleEnumerator(T v) : Enumerator<T>(), value(v) {}
    bool moveNext() {
        switch (this->state) {
            case EnumeratorState::NotStarted:
                this->state = EnumeratorState::Valid;
                return true;
            case EnumeratorState::PastEnd:
                return false;
            case EnumeratorState::Valid:
                this->state = EnumeratorState::PastEnd;
                return false;
        }
        throw std::runtime_error("Unexpected enumerator state");
    }
    T getCurrent() const {
        switch (this->state) {
            case EnumeratorState::NotStarted:
                throw std::logic_error("You cannot call 'getCurrent' before 'moveNext'");
            case EnumeratorState::PastEnd:
                throw std::logic_error("You cannot call 'getCurrent' past the collection end");
            case EnumeratorState::Valid:
                return this->value;
        }
        throw std::runtime_error("Unexpected enumerator state");
    }
};

/////////////////////////////////////////////////////////////////////

/// Always empty iterator (equivalent to end())
template <typename T>
class EmptyEnumerator : public Enumerator<T> {
 public:
    [[nodiscard]] std::string toString() const { return "EmptyEnumerator"; }
    /// Always returns false
    bool moveNext() override { return false; }
    T getCurrent() const override {
        throw std::logic_error("You cannot call 'getCurrent' on an EmptyEnumerator");
    }
};

/////////////////////////////////////////////////////////////////////

/// An enumerator that filters the elements of given inner enumerator.
/// The filter parameter should be a callable object that accepts the wrapped
/// enumerator's value type and returns a bool. When traversing the enumerator,
/// it will call the predicate on each element and skip any where it returns
/// false.
template <typename T, typename Filter>
class FilterEnumerator final : public Enumerator<T> {
    EnumeratorPtr<T> input;
    Filter filter;
    T current;  // must prevent repeated evaluation

 public:
    FilterEnumerator(EnumeratorPtr<T> input, Filter filter)
        : input(input), filter(std::move(filter)) {}

 private:
    bool advance() {
        this->state = EnumeratorState::Valid;
        while (this->input->moveNext()) {
            this->current = this->input->getCurrent();
            bool match = this->filter(this->current);
            if (match) return true;
        }
        this->state = EnumeratorState::PastEnd;
        return false;
    }

 public:
    [[nodiscard]] std::string toString() const {
        return "FilterEnumerator(" + this->input->toString() + "):" + this->stateName();
    }

    void reset() {
        this->input->reset();
        Enumerator<T>::reset();
    }

    bool moveNext() {
        switch (this->state) {
            case EnumeratorState::NotStarted:
            case EnumeratorState::Valid:
                return this->advance();
            case EnumeratorState::PastEnd:
                return false;
        }
        throw std::runtime_error("Unexpected enumerator state");
    }

    T getCurrent() const {
        switch (this->state) {
            case EnumeratorState::NotStarted:
                throw std::logic_error("You cannot call 'getCurrent' before 'moveNext'");
            case EnumeratorState::PastEnd:
                throw std::logic_error("You cannot call 'getCurrent' past the collection end");
            case EnumeratorState::Valid:
                return this->current;
        }
        throw std::runtime_error("Unexpected enumerator state");
    }
};

///////////////////////////

/// Casts each element, preserving ownership when S is a smart pointer.
template <typename T, typename S>
class AsEnumerator final : public Enumerator<S> {
    S getCurrentImpl() const {
        T current = input->getCurrent();
        using Target = typename std::pointer_traits<S>::element_type;
        if constexpr (requires { current->template to<Target>(); }) {
            return current ? current->template to<Target>() : nullptr;
        } else {
            return dynamic_cast<S>(current);
        }
    }

 protected:
    EnumeratorPtr<T> input;

 public:
    explicit AsEnumerator(EnumeratorPtr<T> input) : input(input) {}

    std::string toString() const {
        return "AsEnumerator(" + this->input->toString() + "):" + this->stateName();
    }

    void reset() override {
        Enumerator<S>::reset();
        this->input->reset();
    }

    bool moveNext() override {
        bool result = this->input->moveNext();
        if (result)
            this->state = EnumeratorState::Valid;
        else
            this->state = EnumeratorState::PastEnd;
        return result;
    }

    S getCurrent() const override { return getCurrentImpl(); }
};

///////////////////////////

/// Transforms all elements from type T to type S
template <typename T, typename S, typename Mapper>
class MapEnumerator final : public Enumerator<S> {
 protected:
    EnumeratorPtr<T> input;
    Mapper map;
    S current;

 public:
    MapEnumerator(EnumeratorPtr<T> input, Mapper map) : input(input), map(std::move(map)) {}

    void reset() {
        this->input->reset();
        Enumerator<S>::reset();
    }

    [[nodiscard]] std::string toString() const {
        return "MapEnumerator(" + this->input->toString() + "):" + this->stateName();
    }

    bool moveNext() {
        switch (this->state) {
            case EnumeratorState::NotStarted:
            case EnumeratorState::Valid: {
                bool success = input->moveNext();
                if (success) {
                    T currentInput = this->input->getCurrent();
                    this->current = this->map(currentInput);
                    this->state = EnumeratorState::Valid;
                    return true;
                } else {
                    this->state = EnumeratorState::PastEnd;
                    return false;
                }
            }
            case EnumeratorState::PastEnd:
                return false;
        }
        throw std::runtime_error("Unexpected enumerator state");
    }

    S getCurrent() const {
        switch (this->state) {
            case EnumeratorState::NotStarted:
                throw std::logic_error("You cannot call 'getCurrent' before 'moveNext'");
            case EnumeratorState::PastEnd:
                throw std::logic_error("You cannot call 'getCurrent' past the collection end");
            case EnumeratorState::Valid:
                return this->current;
        }
        throw std::runtime_error("Unexpected enumerator state");
    }
};

template <typename T, typename Mapper>
MapEnumerator(EnumeratorPtr<T>,
              Mapper) -> MapEnumerator<T, typename std::invoke_result_t<Mapper, T>, Mapper>;

/////////////////////////////////////////////////////////////////////

/// Concatenation
template <typename T>
class ConcatEnumerator final : public Enumerator<T> {
    std::vector<EnumeratorPtr<T>> inputs;
    T currentResult;

 public:
    ConcatEnumerator() = default;
    // We take ownership of the vector
    explicit ConcatEnumerator(std::vector<EnumeratorPtr<T>> &&inputs) : inputs(std::move(inputs)) {
        for (const auto &currentInput : inputs)
            if (currentInput == nullptr) throw std::logic_error("Null iterator in concatenation");
    }

    ConcatEnumerator(std::initializer_list<EnumeratorPtr<T>> inputs) : inputs(inputs) {
        for (const auto &currentInput : inputs)
            if (currentInput == nullptr) throw std::logic_error("Null iterator in concatenation");
    }
    explicit ConcatEnumerator(EnumeratorPtr<EnumeratorPtr<T>> inputs)
        : ConcatEnumerator(inputs->toVector()) {}

    [[nodiscard]] std::string toString() const { return "ConcatEnumerator:" + this->stateName(); }

 private:
    bool advance() {
        this->state = EnumeratorState::Valid;
        for (const auto &currentInput : inputs) {
            if (currentInput->moveNext()) {
                this->currentResult = currentInput->getCurrent();
                return true;
            }
        }

        this->state = EnumeratorState::PastEnd;
        return false;
    }

 public:
    EnumeratorPtr<T> concat(EnumeratorPtr<T> other) override {
        // Too late to add
        if (this->state == EnumeratorState::PastEnd)
            throw std::runtime_error("Invalid enumerator state to concatenate");

        inputs.push_back(other);

        return this->shared_from_this();
    }

    void reset() override {
        for (const auto &currentInput : inputs) currentInput->reset();
        Enumerator<T>::reset();
    }

    bool moveNext() override {
        switch (this->state) {
            case EnumeratorState::NotStarted:
            case EnumeratorState::Valid:
                return this->advance();
            case EnumeratorState::PastEnd:
                return false;
        }
        throw std::runtime_error("Unexpected enumerator state");
    }

    T getCurrent() const override {
        switch (this->state) {
            case EnumeratorState::NotStarted:
                throw std::logic_error("You cannot call 'getCurrent' before 'moveNext'");
            case EnumeratorState::PastEnd:
                throw std::logic_error("You cannot call 'getCurrent' past the collection end");
            case EnumeratorState::Valid:
                return this->currentResult;
        }
        throw std::runtime_error("Unexpected enumerator state");
    }
};

////////////////// Enumerator //////////////

template <typename T>
template <typename Mapper>
EnumeratorPtr<std::invoke_result_t<Mapper, T>> Enumerator<T>::map(Mapper map) {
    return std::make_shared<MapEnumerator<T, std::invoke_result_t<Mapper, T>, Mapper>>(
        this->shared_from_this(), std::move(map));
}

template <typename T>
template <typename S>
EnumeratorPtr<S> Enumerator<T>::as() {
    return std::make_shared<AsEnumerator<T, S>>(this->shared_from_this());
}

template <typename T>
template <typename Filter>
EnumeratorPtr<T> Enumerator<T>::where(Filter filter) {
    return std::make_shared<FilterEnumerator<T, Filter>>(this->shared_from_this(),
                                                         std::move(filter));
}

template <typename T>
template <typename Container>
EnumeratorPtr<typename Container::value_type> Enumerator<T>::createEnumerator(
    const Container &data) {
    return std::make_shared<IteratorEnumerator<decltype(data.begin())>>(data.begin(), data.end(),
                                                                        typeid(Container).name());
}

template <typename T>
EnumeratorPtr<T> Enumerator<T>::emptyEnumerator() {
    return std::make_shared<EmptyEnumerator<T>>();
}

template <typename T>
template <typename Iter>
EnumeratorPtr<typename Iter::value_type> Enumerator<T>::createEnumerator(Iter begin, Iter end) {
    return std::make_shared<IteratorEnumerator<Iter>>(begin, end, "iterator");
}

template <typename T>
template <typename Iter>
EnumeratorPtr<typename Iter::value_type> Enumerator<T>::createEnumerator(
    iterator_range<Iter> range) {
    return std::make_shared<IteratorEnumerator<Iter>>(range.begin(), range.end(), "range");
}

template <typename T>
EnumeratorPtr<T> Enumerator<T>::concatAll(EnumeratorPtr<EnumeratorPtr<T>> inputs) {
    return std::make_shared<ConcatEnumerator<T>>(inputs);
}

template <typename T>
EnumeratorPtr<T> Enumerator<T>::concat(EnumeratorPtr<T> other) {
    return std::make_shared<ConcatEnumerator<T>>(
        std::initializer_list<EnumeratorPtr<T>>{this->shared_from_this(), other});
}

///////////////////////////////// EnumeratorHandle ///////////////////

template <typename T>
T EnumeratorHandle<T>::operator*() const {
    if (enumerator == nullptr) throw std::logic_error("Dereferencing end() iterator");
    return enumerator->getCurrent();
}

template <typename T>
const EnumeratorHandle<T> &EnumeratorHandle<T>::operator++() {
    enumerator->moveNext();
    return *this;
}

template <typename T>
bool EnumeratorHandle<T>::operator==(const EnumeratorHandle<T> &other) const {
    return !(*this != other);
}

template <typename T>
bool EnumeratorHandle<T>::operator!=(const EnumeratorHandle<T> &other) const {
    if (this->enumerator == other.enumerator) return true;
    if (other.enumerator != nullptr) throw std::logic_error("Comparison with different iterator");
    return this->enumerator->state == EnumeratorState::Valid;
}

template <typename Iter>
EnumeratorPtr<typename std::iterator_traits<Iter>::value_type> enumerate(Iter begin, Iter end) {
    return std::make_shared<IteratorEnumerator<Iter>>(begin, end, "iterator");
}

template <typename Iter>
EnumeratorPtr<typename std::iterator_traits<Iter>::value_type> enumerate(
    iterator_range<Iter> range) {
    return std::make_shared<IteratorEnumerator<Iter>>(range.begin(), range.end(), "range");
}

template <typename Container>
EnumeratorPtr<typename Container::value_type> enumerate(const Container &data) {
    using std::begin;
    using std::end;
    return std::make_shared<IteratorEnumerator<decltype(begin(data))>>(begin(data), end(data),
                                                                       typeid(data).name());
}

// TODO: Flatten ConcatEnumerator's during concatenation
template <typename T>
EnumeratorPtr<T> concat(std::initializer_list<EnumeratorPtr<T>> inputs) {
    return std::make_shared<ConcatEnumerator<T>>(inputs);
}

template <typename... Args>
auto concat(Args &&...inputs) {
    using FirstEnumeratorTy =
        typename std::decay_t<std::tuple_element_t<0, std::tuple<Args...>>>::element_type;
    std::initializer_list<EnumeratorPtr<typename FirstEnumeratorTy::value_type>> init{
        std::forward<Args>(inputs)...};
    return concat(init);
}

/// Iterate over the handle directly to keep the enumerator alive until the loop finishes.
template <typename T>
EnumeratorHandle<T> begin(const EnumeratorPtr<T> &enumerator) {
    return enumerator->begin();
}

template <typename T>
EnumeratorHandle<T> end(const EnumeratorPtr<T> &enumerator) {
    return enumerator->end();
}

}  // namespace P4::Util

#endif /* LIB_ENUMERATOR_H_ */
