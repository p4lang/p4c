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

#ifndef LIB_ITERATOR_RANGE_H_
#define LIB_ITERATOR_RANGE_H_

#include <iterator>
#include <utility>

namespace Util {
namespace Detail {
using std::begin;
using std::end;

template <typename Container>
using IterOfContainer = decltype(begin(std::declval<Container &>()));

template <typename Range>
constexpr auto begin_impl(Range &&range) {
    return begin(std::forward<Range>(range));
}

template <typename Range>
constexpr auto end_impl(Range &&range) {
    return end(std::forward<Range>(range));
}

}  // namespace Detail

// This is a lightweight alternative for C++20 std::range. Should be replaced
// with ranges as soon as C++20 would be implemented.
template <typename Iter, typename Sentinel = Iter>
class iterator_range {
    using reverse_iterator = std::reverse_iterator<Iter>;

    Iter beginIt, endIt;

 public:
    template <typename Container>
    explicit iterator_range(Container &&c)
        : beginIt(Detail::begin_impl(c)), endIt(Detail::end_impl(c)) {}

    iterator_range(Iter beginIt, Iter endIt)
        : beginIt(std::move(beginIt)), endIt(std::move(endIt)) {}

    auto reverse() const { return iterator_range<reverse_iterator>(rbegin(), rend()); }

    auto begin() const { return beginIt; }
    auto end() const { return endIt; }
    auto rbegin() const { return reverse_iterator{endIt}; }
    auto rend() const { return reverse_iterator{beginIt}; }

    bool empty() const { return beginIt == endIt; }
};

template <typename Container>
iterator_range(Container &&) -> iterator_range<typename Detail::IterOfContainer<Container>>;

}  // namespace Util

#endif /*  LIB_ITERATOR_RANGE_H_ */
