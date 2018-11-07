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

/* -*-c++-*-
   C#-like enumerator interface */

#ifndef P4C_LIB_ENUMERATOR_H_
#define P4C_LIB_ENUMERATOR_H_

#include <vector>
#include <list>
#include <stdexcept>
#include <functional>
#include <cstdint>
#include "lib/cstring.h"

namespace Util {
enum class EnumeratorState {
    NotStarted,
    Valid,
    PastEnd
};

template <typename T> class Enumerator;

// This class provides support for C++-style range for loops
// Enumerator<T>* e;
// for (auto a : *e) ...
template<typename T>
class EnumeratorHandle {
 private:
    Enumerator<T>* enumerator;  // when nullptr it represents end()
    explicit EnumeratorHandle(Enumerator<T>* enumerator) :
            enumerator(enumerator) {}
    friend class Enumerator<T>;

 public:
    T operator* () const;
    const EnumeratorHandle<T>& operator++();
    bool operator != (const EnumeratorHandle<T>& other) const;
};

template <class T>
class Enumerator {
 protected:
    EnumeratorState state = EnumeratorState::NotStarted;

    // This is a weird oddity of C++: this class is a friend of itself with different templates
    template <class S> friend class Enumerator;
    static std::vector<T> emptyVector;
    template <typename S> friend class EnumeratorHandle;

 public:
    Enumerator();
    virtual ~Enumerator() {}

    /* move to next element in the collection;
       return false if the next element does not exist */
    virtual bool moveNext() = 0;
    /* get current element in the collection */
    virtual T getCurrent() const = 0;
    /* move back to the beginning of the collection */
    virtual void reset();

    EnumeratorHandle<T> begin() { this->moveNext(); return EnumeratorHandle<T>(this); }
    EnumeratorHandle<T> end() { return EnumeratorHandle<T>(nullptr); }

    cstring stateName() const {
        switch (this->state) {
            case EnumeratorState::NotStarted:
                return "NotStarted";
            case EnumeratorState::Valid:
                return "Valid";
            case EnumeratorState::PastEnd:
                return "PastEnd";
        }
        throw std::logic_error(cstring("Unexpected state ") +
                               std::to_string(static_cast<int>(this->state)));
    }

    ////////////////// factories
    static Enumerator<T>* createEnumerator(const std::vector<T> &data);
    static Enumerator<T>* createEnumerator(const std::list<T> &data);
    static Enumerator<T>* emptyEnumerator();  // empty data
    template <typename Iter>
    static Enumerator<typename Iter::value_type>* createEnumerator(Iter begin, Iter end);
    // concatenate all these collections into a single one
    static Enumerator<T>* concatAll(Enumerator<Enumerator<T>*>* inputs);

    std::vector<T>* toVector();
    /* Return an enumerator returning all elements that pass the filter */
    Enumerator<T>* where(std::function<bool(const T&)> filter);
    /* Apply specified function to all elements of this enumerator */
    template <typename S>
    Enumerator<S>* map(std::function<S(const T&)> map);
    // cast to an enumerator of S objects
    template<typename S>
    Enumerator<S>* as();
    // append all elements of other after all elements of this
    Enumerator<T>* concat(Enumerator<T>* other);
    /* enumerate all elements and return the count */
    uint64_t count();
    // True if the enumerator has at least one element
    bool any();
    // The only next element; throws if the enumerator does not have exactly 1 element
    T single();
    // Next element, or the default value if none exists
    T nextOrDefault();
    // Next element; throws if there are no elements
    T next();
};

////////////////////////// implementation ///////////////////////////////////////////
// the implementation must be in the header file due to the templates

/* We have to be careful never to invoke getCurrent more than once on each element of the input iterators, since
   it could have side-effects*/

/* A generic iterator returning elements of type T
   C is the container type */

template <typename Iter>
class GenericEnumerator : public Enumerator<typename Iter::value_type> {
 protected:
    Iter begin;
    Iter end;
    Iter current;
    cstring name;
    friend class Enumerator<typename Iter::value_type>;

    GenericEnumerator(Iter begin, Iter end, cstring name)
            : Enumerator<typename Iter::value_type>(),
            begin(begin), end(end), current(begin), name(name) {}

 public:
    cstring toString() const {
        return this->name + ":" + this->stateName();
    }

    bool moveNext() {
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

    typename Iter::value_type getCurrent() const {
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

/////////////////////////////////////////////////////////////////////

/* filters according to a predicate */
template <typename T>
class FilterEnumerator final : public Enumerator<T> {
 protected:
    Enumerator<T>* input;
    std::function<bool(const T&)> filter;
    T current;  // must prevent repeated evaluation

 public:
    FilterEnumerator(Enumerator<T> *input, std::function<bool(const T&)> filter) :
            input(input), filter(filter) {}

 private:
    bool advance() {
        this->state = EnumeratorState::Valid;
        while (this->input->moveNext()) {
            this->current = this->input->getCurrent();
            bool match = this->filter(this->current);
            if (match)
                return true;
        }
        this->state = EnumeratorState::PastEnd;
        return false;
    }

 public:
    cstring toString() const {
        return cstring("FilterEnumerator(") + this->input->toString() + "):" + this->stateName();
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

/* casts each element */
template <typename T, typename S>
class AsEnumerator final : public Enumerator<S> {
 protected:
    Enumerator<T>* input;

 public:
    explicit AsEnumerator(Enumerator<T> *input) :
            input(input) {}

    cstring toString() const {
        return cstring("AsEnumerator(") + this->input->toString() + "):" + this->stateName();
    }

    void reset() {
        Enumerator<S>::reset();
        this->input->reset();
    }

    bool moveNext() {
        bool result = this->input->moveNext();
        if (result)
            this->state = EnumeratorState::Valid;
        else
            this->state = EnumeratorState::PastEnd;
        return result;
    }

    S getCurrent() const {
        T current = input->getCurrent();
        return dynamic_cast<S>(current);
    }
};


///////////////////////////

/* transforms all elements from type T to type S */
template <typename T, typename S>
class MapEnumerator final : public Enumerator<S> {
 protected:
    Enumerator<T>* input;
    std::function<S(const T&)> map;
    S current;

 public:
    MapEnumerator(Enumerator<T>* input, std::function<S(const T&)> map) :
            input(input), map(map) {}

    void reset() {
        this->input->reset();
        Enumerator<S>::reset();
    }

    cstring toString() const {
        return cstring("MapEnumerator(") + this->input->toString() + "):" + this->stateName();
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

/////////////////////////////////////////////////////////////////////

/* concatenation */
template <typename T>
class ConcatEnumerator final : public Enumerator<T> {
 protected:
    Enumerator<Enumerator<T>*>* inputs;
    T currentResult;

 public:
    explicit ConcatEnumerator(Enumerator<Enumerator<T>*>* inputs) :
            inputs(inputs) {}

 private:
    cstring toString() const {
        return "ConcatEnumerator:" + this->stateName();
    }

    bool advance() {
        if (this->state == EnumeratorState::NotStarted) {
            bool start = this->inputs->moveNext();
            if (!start)
                goto theend;
        }

        this->state = EnumeratorState::Valid;
        bool more;
        do {
            Enumerator<T>* currentInput = this->inputs->getCurrent();
            if (currentInput == nullptr)
                throw std::logic_error("Null iterator in concatenation");
            bool moreCurrent = currentInput->moveNext();
            if (moreCurrent) {
                this->currentResult = currentInput->getCurrent();
                return true;
            }

            more = this->inputs->moveNext();
        } while (more);

  theend:
        this->state = EnumeratorState::PastEnd;
        return false;
    }

 public:
    void reset() {
        this->inputs->reset();
        while (this->inputs->moveNext())
            this->inputs->getCurrent()->reset();
        this->inputs->reset();
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
                return this->currentResult;
        }
        throw std::runtime_error("Unexpected enumerator state");
    }
};

////////////////// Enumerator //////////////

template <typename T>
Enumerator<T>::Enumerator() {
    this->reset();
}

template <typename T>
void Enumerator<T>::reset() {
    this->state = EnumeratorState::NotStarted;
}

template <typename T>
template <typename S>
Enumerator<S>* Enumerator<T>::map(std::function<S(const T&)> map) {
    return new MapEnumerator<T, S>(this, map);
}

template <typename T>
template <typename S>
Enumerator<S>* Enumerator<T>::as() {
    return new AsEnumerator<T, S>(this);
}

template <typename T>
Enumerator<T>* Enumerator<T>::where(std::function<bool(const T&)> filter) {
    return new FilterEnumerator<T>(this, filter);
}

template <typename T>
uint64_t Enumerator<T>::count() {
    uint64_t found = 0;
    while (this->moveNext())
        found++;
    return found;
}

template <typename T>
bool Enumerator<T>::any() {
    return this->moveNext();
}

template <typename T>
std::vector<T> Enumerator<T>::emptyVector;

template <typename T>
Enumerator<T>* Enumerator<T>::createEnumerator(const std::vector<T> &data) {
    return new GenericEnumerator<typename std::vector<T>::const_iterator>(
        data.begin(), data.end(), "vector");
}

template <typename T>
Enumerator<T>* Enumerator<T>::emptyEnumerator() {
    return Enumerator<T>::createEnumerator(Enumerator<T>::emptyVector);
}

template <typename T>
Enumerator<T>* Enumerator<T>::createEnumerator(const std::list<T> &data) {
    return new GenericEnumerator<typename std::list<T>::const_iterator>(
        data.begin(), data.end(), "list");
}

template <typename T>
template <typename Iter>
Enumerator<typename Iter::value_type>* Enumerator<T>::createEnumerator(Iter begin, Iter end) {
    return new GenericEnumerator<Iter>(begin, end, "iterator");
}

template <typename T>
std::vector<T>* Enumerator<T>::toVector() {
    auto result = new std::vector<T>();
    while (moveNext())
        result->push_back(getCurrent());
    return result;
}

template <typename T>
Enumerator<T>* Enumerator<T>::concatAll(Enumerator<Enumerator<T>*>* inputs) {
    return new ConcatEnumerator<T>(inputs);
}

template <typename T>
Enumerator<T>* Enumerator<T>::concat(Enumerator<T>* other) {
    std::vector<Enumerator<T>*>* toConcat = new std::vector<Enumerator<T>*>();
    toConcat->push_back(this);
    toConcat->push_back(other);
    Enumerator<Enumerator<T>*>* ce = Enumerator<Enumerator<T>*>::createEnumerator(*toConcat);
    return Enumerator<T>::concatAll(ce);
}

template <typename T>
T Enumerator<T>::next() {
    bool next = moveNext();
    if (!next)
        throw std::logic_error("There is no element for `next()'");
    return getCurrent();
}

template <typename T>
T Enumerator<T>::single() {
    bool next = moveNext();
    if (!next)
        throw std::logic_error("There is no element for `single()'");
    T result = getCurrent();
    next = moveNext();
    if (next)
        throw std::logic_error("There are multiple elements when calling `single()'");
    return result;
}

template <typename T>
T Enumerator<T>::nextOrDefault() {
    bool next = moveNext();
    if (!next)
        return T{};
    return getCurrent();
}

///////////////////////////////// EnumeratorHandle ///////////////////

template <typename T>
T EnumeratorHandle<T>::operator* () const {
    if (enumerator == nullptr)
        throw std::logic_error("Dereferencing end() iterator");
    return enumerator->getCurrent();
}

template <typename T>
const EnumeratorHandle<T>& EnumeratorHandle<T>::operator++() {
    enumerator->moveNext();
    return *this;
}

template <typename T>
bool EnumeratorHandle<T>::operator != (const EnumeratorHandle<T>& other) const {
    if (this->enumerator == other.enumerator) return true;
    if (other.enumerator != nullptr)
        throw std::logic_error("Comparison with different iterator");
    return this->enumerator->state == EnumeratorState::Valid;
}

}  // namespace Util
#endif  /* P4C_LIB_ENUMERATOR_H_ */
