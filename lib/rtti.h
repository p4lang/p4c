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

#ifndef LIB_RTTI_H_
#define LIB_RTTI_H_

#include <algorithm>
#include <array>
#include <cstdint>
#include <type_traits>

namespace RTTI {

using TypeId = std::uint64_t;
static constexpr TypeId InvalidTypeId = UINT64_C(0);

namespace detail {
// Apparently string_view does not optimize properly in constexpr context
// for gcc < 13
struct TypeNameHolder {
    template <size_t N>
    constexpr TypeNameHolder(const char (&str)[N]) : str(&str[0]), length(N - 1) {} // NOLINT(runtime/explicit)

    const char *str;
    size_t length;
};

template <typename T>
constexpr TypeNameHolder TypeNameProxy() {
#ifdef __clang__
    return __PRETTY_FUNCTION__;
#elif defined(__GNUC__)
    return __PRETTY_FUNCTION__;
#else
#error "Unsupported compiler"
#endif
}

static constexpr uint64_t FNV1a(const char *str, size_t n,
                                uint64_t hash = UINT64_C(0xcbf29ce484222325)) {
    return n == 0 ? hash : FNV1a(str + 1, n - 1, UINT64_C(0x100000001b3) * (hash ^ str[0]));
}

static constexpr uint64_t FNV1a(TypeNameHolder str) { return FNV1a(str.str, str.length); }

// Default implementation of typeid resolver: use hash derived from type name,
// but mark upper 8 bits to be 0xFF
template <typename T, typename = void>
struct TypeIdResolver {
    static constexpr TypeId resolve() noexcept {
        // Get original 64-bit hash
        uint64_t hash = FNV1a(TypeNameProxy<T>());
        // Xor-fold to obtain 56-bit value
        hash = (hash >> 56) ^ (hash & ((UINT64_C(1) << 56) - 1));
        return (UINT64_C(0xFF) << 56) | hash;
    }
};

// Specialized implementation that looks for static_typeId member.
template <typename T>
struct TypeIdResolver<T, std::void_t<decltype(T::static_typeId)>> {
    static constexpr uint64_t resolve() noexcept {
        constexpr TypeId id = T::static_typeId();
        // Allow fall-back implementation if class does not want to explicitly
        // specify a typeid
        if constexpr (id == InvalidTypeId) return TypeIdResolver<T, int>::resolve();

        return id;
    }
};

}  // namespace detail

struct Base;

template <typename This, typename... Parents>
struct TypeInfo {
    using T = std::remove_const_t<std::remove_reference_t<This>>;

    static_assert((... && std::is_base_of_v<Parents, This>),
                  "One or more parents are not a base of this type.");

    static_assert((... && std::is_base_of_v<Base, Parents>),
                  "One or more parent hierarchies is not based on top of RTTI::Base.");

    static_assert(std::is_base_of_v<Base, This>,
                  "Invalid typeinfo requested, class is not based on top of RTTI::Base.");

    static_assert(
        std::is_same_v<T *, decltype(T::rttiEnabledMarker(nullptr))>,
        "Invalid typeinfo requested, class does not declare typeinfo via DECLARE_TYPEINFO.");

    [[nodiscard]] static constexpr TypeId id() noexcept {
        return detail::TypeIdResolver<T>::resolve();
    }

    [[nodiscard]] static constexpr bool isA(TypeId typeId) noexcept {
        return (id() == typeId) || (... || (Parents::TypeInfo::isA(typeId)));
    }

    template <typename T>
    [[nodiscard]] static const void *dyn_cast(TypeId typeId, const T *ptr) noexcept {
        if (id() == typeId) return static_cast<const This *>(ptr);

        std::array<const void *, sizeof...(Parents)> ptrs = {
            Parents::TypeInfo::dyn_cast(typeId, ptr)...};

        auto it =
            std::find_if(ptrs.begin(), ptrs.end(), [](const void *ptr) { return ptr != nullptr; });
        return (it != ptrs.end()) ? *it : nullptr;
    }
};

struct Base {
    virtual ~Base() = default;

    [[nodiscard]] virtual TypeId typeId() const noexcept = 0;
    [[nodiscard]] virtual bool isA(TypeId typeId) const noexcept = 0;

    template <typename T>
    [[nodiscard]] bool is() const noexcept {
        return isA(TypeInfo<T>::id());
    }

    template <typename T>
    [[nodiscard]] T *to() noexcept {
        return reinterpret_cast<T *>(const_cast<void *>(toImpl(TypeInfo<T>::id())));
    }

    template <typename T>
    [[nodiscard]] const T *to() const noexcept {
        return reinterpret_cast<const T *>(toImpl(TypeInfo<T>::id()));
    }

 protected:
    [[nodiscard]] virtual const void *toImpl(TypeId typeId) const noexcept = 0;
};

}  // namespace RTTI

#define DECLARE_TYPEINFO(T, ...)                                                  \
 private:                                                                         \
    static constexpr RTTI::TypeId static_typeId() { return RTTI::InvalidTypeId; } \
    DECLARE_TYPEINFO_COMMON(T, ##__VA_ARGS__)

#define DECLARE_TYPEINFO_WITH_TYPEID(T, Id, ...)                 \
 public:                                                         \
    static constexpr RTTI::TypeId static_typeId() { return Id; } \
    DECLARE_TYPEINFO_COMMON(T, ##__VA_ARGS__)

#define DECLARE_TYPEINFO_WITH_NESTED_TYPEID(T, OuterId, Id, ...)            \
 public:                                                                    \
    static constexpr RTTI::TypeId static_typeId() { return OuterId | Id; }; \
    DECLARE_TYPEINFO_COMMON(T, ##__VA_ARGS__)

#define DECLARE_TYPEINFO_COMMON(T, ...)                                                    \
 public:                                                                                   \
    static constexpr T *rttiEnabledMarker(T *);                                            \
    using TypeInfo = RTTI::TypeInfo<T, ##__VA_ARGS__>;                                     \
    [[nodiscard]] RTTI::TypeId typeId() const noexcept override { return TypeInfo::id(); } \
    [[nodiscard]] bool isA(RTTI::TypeId typeId) const noexcept override {                  \
        return TypeInfo::isA(typeId);                                                      \
    }                                                                                      \
                                                                                           \
 protected:                                                                                \
    [[nodiscard]] const void *toImpl(RTTI::TypeId typeId) const noexcept override {        \
        return TypeInfo::isA(typeId) ? TypeInfo::dyn_cast(typeId, this) : nullptr;         \
    }

#endif /* LIB_RTTI_H_ */
