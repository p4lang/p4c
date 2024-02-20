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
// FIXME: We might have begin() found via ADL, support it as well
template <typename Container>
using IterOfContainer = decltype(std::begin(std::declval<Container &>()));
}  // namespace Detail

template <typename Iter, typename Sentinel = Iter>
class iterator_range {
    using reverse_iterator = std::reverse_iterator<Iter>;

    Iter beginIt, endIt;

 public:
    template <typename Container>
    explicit iterator_range(Container &&c) : beginIt(c.begin()), endIt(c.end()) {}

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
