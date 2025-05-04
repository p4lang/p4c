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

#ifndef LIB_BOOST_FORMAT_COMPAT_H_
#define LIB_BOOST_FORMAT_COMPAT_H_

#include <boost/format.hpp>

namespace P4 {

/// Allocator wrapper that adds the overload removed in C++20 for the sake of Boost.
template <typename T>
struct BoostCompatAllocator : std::allocator<T> {
    /// Inherit constructors.
    using std::allocator<T>::allocator;

    BoostCompatAllocator() = default;
    /// Explictly make it possible to construct our allocator from std::allocator.
    /// Some parts of libstdc++ 9 require that for some reason.
    BoostCompatAllocator(const std::allocator<T> &other)
        : std::allocator<T>(other) {}  // NOLINT(runtime/explicit)
    BoostCompatAllocator(std::allocator<T> &&other)
        : std::allocator<T>(std::move(other)) {}  // NOLINT(runtime/explicit)

    /// This one was removed in C++20, but boost::format < 1.74 needs it.
    T *allocate(typename std::allocator<T>::size_type n, const void *) { return allocate(n); }

    /// Explicitly wrap the other overload (instead of using using std::allocator<T>::allocate.
    /// The latter does not work with GCC 9.
    T *allocate(typename std::allocator<T>::size_type n) { return std::allocator<T>::allocate(n); }
};

/// Boost < 1.74 is not compatible with C++20 as it expect deprecated
/// allocator::allocate(size_type, void*) to exist. To work around that, we use a simple wrapper
/// over std::allocator that adds this overload back. All uses of boost::format in P4C should use
// this type instead.
using BoostFormatCompat =
    boost::basic_format<char, std::char_traits<char>, BoostCompatAllocator<char>>;

}  // namespace P4

#endif  // LIB_BOOST_FORMAT_COMPAT_H_
