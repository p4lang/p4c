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

#ifndef LIB_BITRANGE_H_
#define LIB_BITRANGE_H_

#include <algorithm>
#include <iosfwd>
#include <limits>
#include <optional>
#include <utility>

#include "absl/numeric/bits.h"
#include "bitvec.h"
#include "exceptions.h"
#include "hash.h"

/* iterate over ranges of contiguous bits in a bitvector */
class bitranges {
    bitvec tmp;
    const bitvec &bits;
    struct iter {
        bool valid = true;
        bitvec::const_bitref ptr;
        std::pair<int, int> range;

        iter &operator++() {
            if (ptr) {
                range.first = range.second = ptr.index();
                while (++ptr && range.second + 1 == ptr.index()) ++range.second;
            } else {
                valid = false;
            }
            return *this;
        }
        std::pair<int, int> operator*() { return range; }
        bool operator==(iter &a) const { return valid == a.valid && ptr == a.ptr; }
        bool operator!=(iter &a) const { return !(*this == a); }
        explicit iter(bitvec::const_bitref p) : ptr(p) { ++*this; }
    };

 public:
    explicit bitranges(const bitvec &b) : bits(b) {}
    explicit bitranges(bitvec &&b) : tmp(b), bits(tmp) {}
    explicit bitranges(uintptr_t b) : tmp(b), bits(tmp) {}
    iter begin() const { return iter(bits.begin()); }
    iter end() const { return iter(bits.end()); }
};

class JSONGenerator;
class JSONLoader;

namespace BitRange {

namespace Detail {
/**
 * @return the result of dividing @dividend by @divisor, rounded towards
 * negative infinity. This is different than normal C++ integer division, which
 * rounds towards zero. For example, `-7 / 8 == 0`, but
 * `divideFloor(-7, 8) == -1`.
 */
constexpr inline int divideFloor(int dividend, int divisor) {
#if defined(__GNUC__) || defined(__clang__)
    // Code to enable compiler folding when the divisor is a power-of-two compile-time constant
    // In this case, compiler should fold to a right-shift
    // FIXME: Replace absl with std after moving to C++20
    unsigned u_divisor = static_cast<unsigned>(divisor);
    if (__builtin_constant_p(u_divisor) && absl::has_single_bit(u_divisor))
        return dividend >> (absl::bit_width(u_divisor) - 1);
#endif  // defined(__GNUC__) || defined(__clang__)

    const int quotient = dividend / divisor;
    const int remainder = dividend % divisor;
    if ((remainder != 0) && ((remainder < 0) != (divisor < 0))) return quotient - 1;
    return quotient;
}

/**
 * @return @dividend modulo @divisor as distinct from C++'s
 * `dividend % divisor`. The difference is that the result of
 * `dividend % divisor` takes its sign from the dividend, but
 * `modulo(dividend, divisor)` takes its sign from the divisor.
 * For example, `-7 % 8 == -7`, but `modulo(-7, 8) == 7`.
 */
constexpr int modulo(int dividend, int divisor) {
    return (dividend % divisor) * ((dividend < 0) != (divisor < 0) ? -1 : 1);
}

/**
 * @return the remainder of dividing @dividend by @divisor, rounded towards
 * negative infinity. This is different than the normal C++ `dividend % divisor` in
 * two respects:
 *   - As with `modulo(dividend, divisor)`, it takes its sign from the divisor.
 *   - For all integers `D1`, `D2`, `D3`,
 *     `divideFloor(D1, D3) == divideFloor(D2, D3) /\ D1 < D2
 *          ==> moduloFloor(D1, D3) < moduloFloor(D2, D3)`.
 *     This is not true for `%`, because due to C++'s round-towards-zero
 *     behavior the remainders grow in different directions for positive
 *     dividends than for negative dividends.
 * To make this concrete, `-7 % 8 == -7`, but `moduloFloor(-7, 8) == 1`.
 */
constexpr inline int moduloFloor(const int dividend, const int divisor) {
#if defined(__GNUC__) || defined(__clang__)
    // Code to enable compiler folding when the divisor is a power-of-two compile-time constant
    // In this case, compiler should fold to a bitwise-and
    // FIXME: Replace absl with std after moving to C++20
    if (__builtin_constant_p(divisor) && absl::has_single_bit(static_cast<unsigned>(divisor)))
        return dividend & (divisor - 1);
#endif  // defined(__GNUC__) || defined(__clang__)

    const int remainder = modulo(dividend, divisor);
    if (remainder == 0 || dividend >= 0) return remainder;
    return divisor - remainder;
}
}  // namespace Detail

/**
 * A helper type used to construct a range, specified as the closed interval
 * between two bit indices. For example, `FromTo(6, 8)` denotes a range
 * containing three bits: bit 6, bit 7, and bit 8.
 *
 * FromTo is only intended to be used as a constructor parameter, to allow
 * creation of ranges in a more readable manner; use it to construct a
 * HalfOpenRange or ClosedRange.
 */
struct FromTo {
    FromTo(int from, int to) : from(from), to(to) {}
    FromTo(const FromTo &) = delete;
    FromTo &operator=(const FromTo &) = delete;
    const int from;
    const int to;
};

/**
 * A helper type used to construct a range, specified as the region of a certain
 * length beginning at a certain bit index. For example, `StartLen(6, 8)`
 * denotes a range containing eight bits: bit 6, bit 7, bit 8, bit 9, bit 10,
 * bit 11, bit 12, and bit 13.
 *
 * As with FromTo, StartLen is only meant to be used when constructing a
 * HalfOpenRange or ClosedRange.
 */
struct StartLen {
    StartLen(int start, int len) : start(start), len(len) {}
    StartLen(const StartLen &) = delete;
    StartLen &operator=(const StartLen &) = delete;
    const int start;
    const int len;
};

/**
 * A helper type used to construct a range that starts at zero and ends at the
 * maximum index that is safely representable without overflow. Only meant to be
 * used when constructing a HalfOpenRange or ClosedRange.
 *
 * ZeroToMax ranges are useful as an identity when intersecting ranges. If you
 * have a loop where you intersect ranges over and over, you may find ZeroToMax
 * useful.
 *
 * A caveat: "safely representable" above means that basic bitrange getters,
 * queries, comparisons, and set operations work. It does *not* mean that any
 * computation you do with the bitrange will be safe; operations that increase
 * the size of very large bitranges, that shift them, or that change their units
 * may result in integer overflow.
 *
 * It's generally safe to change the endianness of ZeroToMax, as long as the
 * space it lives inside is safe. If you try to treat it as living in a space
 * which also has a very large size, you may encounter integer overflow.
 *
 * XXX: There's no substitute for checking unsafe conversions before
 * performing them, but we don't do that right now, so be cautious with very
 * large ranges, just as you would be with storing very large values into a
 * primitive type.
 */
struct ZeroToMax {};

/**
 * A helper type used to construct a range that starts at the minimum index that
 * is safely representable without overflow and ends at the maximum index that
 * is safely representable without overflow. Only meant to be used when
 * constructing a HalfOpenRange or ClosedRange.
 *
 * MinToMax ranges are useful as an identity when intersecting ranges. If you
 * have a loop where you intersect ranges over and over, you may find MinToMax
 * useful. Note that you're less likely to run into overflow issues with
 * ZeroToMax ranges, so if you're dealing entirely with non-negative numbers,
 * you're probably better off using ZeroToMax.
 *
 * @see ZeroToMax for more discussion about what "safely representable" means.
 *
 * Unlike ZeroToMax, it is not safe to change the endianness of MinToMax; doing
 * so will inevitably result in integer overflow.
 */
struct MinToMax {};

/// JSON serialization/deserialization helpers.
void rangeToJSON(JSONGenerator &json, int lo, int hi);
std::pair<int, int> rangeFromJSON(JSONLoader &json);

}  // namespace BitRange

/// Units in which a range can be specified.
enum class RangeUnit : uint8_t {
    Bit = 0,
    Byte = 1  /// 8-bit bytes (octets).
};

/// An ordering for bits or bytes.
enum class Endian : uint8_t {
    Network = 0,  /// Most significant bit/byte first.
    Little = 1,   /// Least significant bit/byte first.
};

/**
 * A half-open range of bits or bytes - `[lo, hi)` - specified in terms of a
 * specific endian order. Half-open ranges include `lo` but do not include `hi`,
 * so `HalfOpenRange(3, 5)` contains `3` and `4` but not `5`.
 *
 * Use a half-open range when you want to allow for the possibility that the
 * range may be empty, which may be represented by setting `lo` and `hi` to the
 * same value. Using a half-open range may also make some algorithms more
 * natural to express, and it may make working with external code easier since
 * half-open ranges are idiomatic in C++.
 *
 * Note that there are many ways to represent an empty range - `(1, 1)` is
 * empty, for example, but so is `(2, 2)`. Many operations on HalfOpenRanges
 * will canonicalize the empty range representation, so don't rely on it - just
 * call `empty()` to determine if a range is empty.
 *
 * XXX: Currently, for backwards compatibility, it's possible to construct
 * ranges where `lo` is greater than `hi`. We should enforce that ranges are
 * consistent; we'll add the necessary checks after the existing code has been
 * audited.
 *
 * XXX: We should also add checks to avoid integer overflow.
 */
template <RangeUnit Unit, Endian Order>
struct HalfOpenRange {
    static constexpr RangeUnit unit = Unit;
    static constexpr Endian order = Order;

    using FromTo = BitRange::FromTo;
    using StartLen = BitRange::StartLen;
    using ZeroToMax = BitRange::ZeroToMax;
    using MinToMax = BitRange::MinToMax;

    HalfOpenRange() : lo(0), hi(0) {}
    HalfOpenRange(int lo, int hi) : lo(lo), hi(hi) {}
    HalfOpenRange(FromTo &&fromTo)  // NOLINT(runtime/explicit)
        : lo(fromTo.from), hi(fromTo.to + 1) {}
    HalfOpenRange(StartLen &&startLen)  // NOLINT(runtime/explicit)
        : lo(startLen.start), hi(startLen.start + startLen.len) {}
    HalfOpenRange(ZeroToMax &&)  // NOLINT(runtime/explicit)
        : lo(0), hi(std::numeric_limits<int>::max()) {}
    HalfOpenRange(MinToMax &&)  // NOLINT(runtime/explicit)
        : lo(std::numeric_limits<int>::min()), hi(std::numeric_limits<int>::max()) {}
    explicit HalfOpenRange(std::pair<int, int> range) : lo(range.first), hi(range.second) {}

    /// @return the number of elements in this range.
    ssize_t size() const { return ssize_t(hi) - ssize_t(lo); }

    /// @return a canonicalized version of this range. This only has an effect
    /// for empty ranges, which will all be mapped to (0, 0).
    HalfOpenRange canonicalize() const {
        if (empty()) return HalfOpenRange(0, 0);
        return *this;
    }

    /// @return a new range with the same starting index, but expanded or
    /// contracted so that it has the provided size in bits. The end result is
    /// always in bits.
    HalfOpenRange<RangeUnit::Bit, Order> resizedToBits(int size) const {
        if (empty()) return HalfOpenRange<RangeUnit::Bit, Order>(0, size);
        auto asBits = toUnit<RangeUnit::Bit>();
        return {asBits.lo, asBits.lo + size};
    }

    /// @return a new range with the same starting index, but expanded or
    /// contracted so that it has the provided size in bytes.
    HalfOpenRange resizedToBytes(int size) const {
        const int resizedLo = empty() ? 0 : lo;
        if (Unit == RangeUnit::Byte) return {resizedLo, resizedLo + size};
        return {resizedLo, resizedLo + size * 8};
    }

    /// @return a new range with the same size, but shifted towards the
    /// high-numbered bits by the provided amount. No rotation or clamping
    /// to zero is applied. The result is always in bits.
    HalfOpenRange<RangeUnit::Bit, Order> shiftedByBits(int offset) const {
        if (empty()) return HalfOpenRange<RangeUnit::Bit, Order>();
        auto asBits = toUnit<RangeUnit::Bit>();
        return {asBits.lo + offset, asBits.hi + offset};
    }

    /// @return a new range with the same size, but shifted towards the
    /// high-numbered bytes by the provided amount. No rotation or clamping
    /// to zero is applied.
    HalfOpenRange<Unit, Order> shiftedByBytes(int offset) const {
        if (empty()) return HalfOpenRange();
        if (Unit == RangeUnit::Byte) return {lo + offset, hi + offset};
        return {lo + offset * 8, hi + offset * 8};
    }

    /// @return the byte containing the lowest-numbered bit in this interval.
    int loByte() const {
        if (empty()) return 0;
        return Unit == RangeUnit::Byte ? lo : BitRange::Detail::divideFloor(lo, 8);
    }

    /// @return the byte containing the highest-numbered bit in this interval.
    int hiByte() const {
        if (empty()) return 0;
        return Unit == RangeUnit::Byte ? hi - 1 : BitRange::Detail::divideFloor(hi - 1, 8);
    }

    /// @return the next byte that starts after the end of this interval. May
    /// result in integer overflow if used with ZeroToMax or MinToMax-sized
    /// ranges.
    int nextByte() const { return empty() ? 0 : hiByte() + 1; }

    /// @return true if the lowest-numbered bit in this range is
    /// byte-aligned.
    bool isLoAligned() const {
        return (empty() || Unit == RangeUnit::Byte) ? true
                                                    : BitRange::Detail::moduloFloor(lo, 8) == 0;
    }

    /// @return true if the highest-numbered bit in this range is
    /// byte-aligned (meaning that this range stops right before the
    /// beginning of a new byte).
    bool isHiAligned() const {
        return (empty() || Unit == RangeUnit::Byte) ? true
                                                    : BitRange::Detail::moduloFloor(hi, 8) == 0;
    }

    bool operator==(HalfOpenRange other) const {
        if (empty()) return other.empty();
        return other.lo == lo && other.hi == hi;
    }
    bool operator!=(HalfOpenRange other) const { return !(*this == other); }

    /// @return true if this range is empty - i.e., it contains no elements.
    bool empty() const { return lo == hi; }

    /// @return true if this range includes the provided index.
    bool contains(int index) const { return index >= lo && index < hi; }

    /// @return true if this range includes the provided range - i.e., if this
    /// range contains a superset of the provided range's bits.
    bool contains(HalfOpenRange other) const { return intersectWith(other) == other; }

    /// @return true if this range has some bits in common with the provided
    /// range. Note that an empty range never overlaps with any other range, so
    /// `rangeA == rangeB` does not imply that `rangeA.overlaps(rangeB)`.
    bool overlaps(HalfOpenRange a) const { return !intersectWith(a).empty(); }
    bool overlaps(int l, int h) const { return !intersectWith(l, h).empty(); }

    /// @return a range which contains all the bits which are included in both
    /// this range and the provided range, or an empty range if there are no bits
    /// in common.
    HalfOpenRange intersectWith(HalfOpenRange a) const { return intersectWith(a.lo, a.hi); }
    HalfOpenRange intersectWith(int l, int h) const {
        HalfOpenRange rv = {std::max(lo, l), std::min(hi, h)};
        if (rv.hi <= rv.lo) return {0, 0};
        return rv;
    }
    HalfOpenRange operator&(HalfOpenRange a) const { return intersectWith(a); }
    HalfOpenRange operator&=(HalfOpenRange a) {
        *this = intersectWith(a);
        return *this;
    }

    /// @return the smallest range that contains all of the bits in both this
    /// range and the provided range. Note that because ranges are contiguous,
    /// the result may contain bits that are not in either of the original
    /// ranges.
    HalfOpenRange unionWith(HalfOpenRange a) const { return unionWith(a.lo, a.hi); }
    HalfOpenRange unionWith(int l, int h) const {
        if (empty()) return {l, h};
        if (l == h) return *this;
        return HalfOpenRange(std::min(lo, l), std::max(hi, h));
    }
    HalfOpenRange operator|(HalfOpenRange a) const { return unionWith(a); }
    HalfOpenRange operator|=(HalfOpenRange a) {
        *this = unionWith(a);
        return *this;
    }

    /**
     * Convert this range to a range with the specified endian order.
     *
     * Making this conversion requires specifying the size of the larger space
     * that this range is embedded in. That's because the place we start
     * numbering indices from (the origin, in other words) is changing.  For
     * example, given a space with 10 elements, a range of 3 indices within that
     * space may be numbered in two ways as follows:
     *             Space:               [o o o o o o o o o o]
     *             Big endian range:    --> [2 3 4]
     *             Little endian range:     [7 6 5] <--------
     * We couldn't switch between those numberings without knowing that the
     * space has 10 elements.
     *
     * Note that this operation may result in integer overflow when dealing with
     * very large ranges. It's generally safe for ZeroToMax as long as the space
     * size is not also near INT_MAX. It's never safe for MinToMax.
     *
     * @tparam DestOrder  The endian order to convert to.
     * @param spaceSize  The size of the space this range is embedded in.
     * @return this range, but converted to the specified endian ordering.
     */
    template <Endian DestOrder>
    HalfOpenRange<Unit, DestOrder> toOrder(int spaceSize) const {
        if (DestOrder == Order) return HalfOpenRange<Unit, DestOrder>(lo, hi);
        switch (DestOrder) {
            case Endian::Network:
            case Endian::Little:
                return HalfOpenRange<Unit, DestOrder>(spaceSize - hi, spaceSize - lo);
        }
        BUG("Unexpected ordering");
    }

    /**
     * Convert this range to the smallest enclosing range with the specified unit.
     *
     * Conversion from bytes to bits is exact, but conversion from bits to bytes
     * is lossy and may cause the size of the range to grow if the bits aren't
     * byte-aligned. If that would be problematic, use `isLoAligned()` and
     * `isHiAligned()` to check before converting, or just check that converting
     * the result back to bits yields the original range.
     *
     * This operation will result in integer overflow when dealing with
     * ZeroToMax or MinToMax-sized ranges.
     *
     * @tparam DestUnit  The unit to convert to.
     * @return this range, but converted to the specified unit.
     */
    template <RangeUnit DestUnit>
    HalfOpenRange<DestUnit, Order> toUnit() const {
        if (DestUnit == Unit) return HalfOpenRange<DestUnit, Order>(lo, hi);
        if (empty()) return HalfOpenRange<DestUnit, Order>();
        switch (DestUnit) {
            case RangeUnit::Bit:
                return HalfOpenRange<DestUnit, Order>(lo * 8, hi * 8);
            case RangeUnit::Byte:
                return HalfOpenRange<DestUnit, Order>(loByte(), nextByte());
        }
        BUG("Unexpected unit");
    }

    /// JSON serialization/deserialization.
    void toJSON(JSONGenerator &json) const { BitRange::rangeToJSON(json, lo, hi); }
    static HalfOpenRange fromJSON(JSONLoader &json) {
        return HalfOpenRange(BitRange::rangeFromJSON(json));
    }

    /// Total ordering, first by lo, then by hi.
    bool operator<(const HalfOpenRange &other) const {
        if (lo != other.lo) return lo < other.lo;
        return hi < other.hi;
    }

    friend size_t hash_value(const HalfOpenRange &r) { return Util::Hash{}(r.lo, r.hi); }

    /// The lowest numbered index in the range. For Endian::Network, this is the
    /// most significant bit or byte; for Endian::Little, it's the least
    /// significant.
    int lo;

    /// The highest numbered index in the range. For Endian::Network, this is the
    /// least significant bit or byte; for Endian::Little, it's the most
    /// significant. Because this is a half-open range, the range element this
    /// index identifies is not included in the range.
    int hi;
};

/**
 * A closed range of bits or bytes - `[lo, hi]` specified in terms of a specific
 * endian order. Closed ranges include both `lo` and `hi`, so
 * `ClosedRange(3, 5)` contains `3`, `4`, and `5`.
 *
 * Use a closed range when you want to forbid the possibility of an empty range.
 * Using a closed range may also make certain algorithms easier to express.
 *
 * XXX: ClosedRange's interface is very similar to HalfOpenRange, but they
 * have almost totally different implementations, and the limitations of C++
 * templates make sharing the things they do have in common challenging. Most of
 * the benefit would have come from sharing documentation; as a compromise, most
 * of the methods in ClosedRange just reference HalfOpenRange's documentation.
 *
 * XXX: Currently, for backwards compatibility, it's possible to construct
 * ranges where `lo` is greater than or equal to `hi`. We should enforce that
 * ranges are consistent; we'll add the necessary checks after the existing code
 * has been audited.
 *
 * XXX: We should also add checks to avoid integer overflow.
 */
template <RangeUnit Unit, Endian Order>
struct ClosedRange {
    static constexpr RangeUnit unit = Unit;
    static constexpr Endian order = Order;

    using FromTo = BitRange::FromTo;
    using StartLen = BitRange::StartLen;
    using ZeroToMax = BitRange::ZeroToMax;
    using MinToMax = BitRange::MinToMax;

    ClosedRange() : lo(0), hi(0) {}  // FIXME: default is [0,0]? This is just wrong ...
    constexpr ClosedRange(int lo, int hi) : lo(lo), hi(hi) {}
    ClosedRange(FromTo &&fromTo)  // NOLINT(runtime/explicit)
        : lo(fromTo.from), hi(fromTo.to) {}
    ClosedRange(StartLen &&startLen)  // NOLINT(runtime/explicit)
        : lo(startLen.start), hi(startLen.start + startLen.len - 1) {}
    ClosedRange(ZeroToMax &&)  // NOLINT(runtime/explicit)
        : lo(0), hi(std::numeric_limits<int>::max() - 1) {}
    ClosedRange(MinToMax &&)  // NOLINT(runtime/explicit)
        : lo(std::numeric_limits<int>::min()), hi(std::numeric_limits<int>::max() - 1) {}
    explicit ClosedRange(std::pair<int, int> range) : lo(range.first), hi(range.second) {}
    ClosedRange(const HalfOpenRange<Unit, Order> &r)  // NOLINT(runtime/explicit)
        : lo(r.lo), hi(r.hi - 1) {
        BUG_CHECK(!r.empty(), "can't convert empty range to Closed");
    }

    /// @see HalfOpenRange::size().
    ssize_t size() const { return ssize_t(hi) - ssize_t(lo) + 1; }

    /// @see HalfOpenRange::resizedToBits().
    ClosedRange<RangeUnit::Bit, Order> resizedToBits(int size) const {
        BUG_CHECK(size != 0, "Resizing ClosedRange to zero size");
        auto asBits = toUnit<RangeUnit::Bit>();
        return {asBits.lo, asBits.lo + size - 1};
    }

    /// @see HalfOpenRange::resizedToBytes().
    ClosedRange resizedToBytes(int size) const {
        BUG_CHECK(size != 0, "Resizing ClosedRange to zero size");
        if (Unit == RangeUnit::Byte) return {lo, lo + size - 1};
        return {lo, lo + size * 8 - 1};
    }

    /// @see HalfOpenRange::shiftedByBits().
    ClosedRange<RangeUnit::Bit, Order> shiftedByBits(int offset) const {
        auto asBits = toUnit<RangeUnit::Bit>();
        return {asBits.lo + offset, asBits.hi + offset};
    }

    /// @see HalfOpenRange::shiftedByBytes().
    ClosedRange shiftedByBytes(int offset) const {
        if (Unit == RangeUnit::Byte) return {lo + offset, hi + offset};
        return {lo + offset * 8, hi + offset * 8};
    }

    /// @see HalfOpenRange::loByte().
    int loByte() const {
        return Unit == RangeUnit::Byte ? lo : BitRange::Detail::divideFloor(lo, 8);
    }

    /// @see HalfOpenRange::hiByte().
    int hiByte() const {
        return Unit == RangeUnit::Byte ? hi : BitRange::Detail::divideFloor(hi, 8);
    }

    /// @see HalfOpenRange::nextByte().
    int nextByte() const { return hiByte() + 1; }

    /// @see HalfOpenRange::isLoAligned().
    bool isLoAligned() const {
        return Unit == RangeUnit::Byte ? true : BitRange::Detail::moduloFloor(lo, 8) == 0;
    }

    /// @see HalfOpenRange::isHiAligned().
    bool isHiAligned() const {
        return Unit == RangeUnit::Byte ? true : BitRange::Detail::moduloFloor(hi + 1, 8) == 0;
    }

    bool operator==(ClosedRange other) const { return (other.lo == lo) && (other.hi == hi); }
    bool operator!=(ClosedRange other) const { return !(*this == other); }

    /// @see HalfOpenRange::contains().
    bool contains(int index) const { return (index >= lo) && (index <= hi); }

    /// @see HalfOpenRange::contains().
    bool contains(ClosedRange other) const {
        auto intersection = intersectWith(other);
        return intersection.lo == other.lo && intersection.size() == other.size();
    }

    /// @see HalfOpenRange::overlaps().
    bool overlaps(ClosedRange a) const { return !intersectWith(a).empty(); }
    bool overlaps(int l, int h) const { return !intersectWith(l, h).empty(); }

    /// @return a range which contains all the bits which are included in both
    /// this range and the provided range, or an empty range if there are no bits
    /// in common. Because the resulting range may be empty, this method returns
    /// a HalfOpenRange.
    HalfOpenRange<Unit, Order> intersectWith(ClosedRange a) const {
        return intersectWith(a.lo, a.hi);
    }
    HalfOpenRange<Unit, Order> intersectWith(int l, int h) const {
        return HalfOpenRange<Unit, Order>(lo, hi + 1)
            .intersectWith(HalfOpenRange<Unit, Order>(l, h + 1));
    }
    HalfOpenRange<Unit, Order> operator&(ClosedRange a) const { return intersectWith(a); }
    HalfOpenRange<Unit, Order> operator&=(ClosedRange a) {
        *this = intersectWith(a);
        return *this;
    }

    /// @see HalfOpenRange::unionWith().
    ClosedRange unionWith(ClosedRange a) const { return unionWith(a.lo, a.hi); }
    ClosedRange unionWith(int l, int h) const {
        return ClosedRange(std::min(lo, l), std::max(hi, h));
    }
    ClosedRange operator|(ClosedRange a) const { return unionWith(a); }
    ClosedRange operator|=(ClosedRange a) {
        *this = unionWith(a);
        return *this;
    }

    /// @see HalfOpenRange::toOrder().
    template <Endian DestOrder>
    ClosedRange<Unit, DestOrder> toOrder(int spaceSize) const {
        BUG_CHECK(spaceSize > 0, "Can't represent an empty range");
        if (DestOrder == Order) return ClosedRange<Unit, DestOrder>(lo, hi);
        switch (DestOrder) {
            case Endian::Network:
            case Endian::Little:
                return ClosedRange<Unit, DestOrder>((spaceSize - 1) - hi, (spaceSize - 1) - lo);
        }
        BUG("Unexpected ordering");
    }

    /// @see HalfOpenRange::toUnit().
    template <RangeUnit DestUnit>
    ClosedRange<DestUnit, Order> toUnit() const {
        if (DestUnit == Unit) return ClosedRange<DestUnit, Order>(lo, hi);
        switch (DestUnit) {
            case RangeUnit::Bit:
                return ClosedRange<DestUnit, Order>(lo * 8, hi * 8 + 7);
            case RangeUnit::Byte:
                return ClosedRange<DestUnit, Order>(loByte(), hiByte());
        }
        BUG("Unexpected unit");
    }

    /// JSON serialization/deserialization.
    void toJSON(JSONGenerator &json) const { BitRange::rangeToJSON(json, lo, hi); }
    static ClosedRange fromJSON(JSONLoader &json) {
        return ClosedRange(BitRange::rangeFromJSON(json));
    }

    /// @see HalfOpenRange::operator<.
    bool operator<(const ClosedRange &other) const {
        if (lo != other.lo) return lo < other.lo;
        return hi < other.hi;
    }

    friend size_t hash_value(const ClosedRange &r) { return Util::Hash{}(r.lo, r.hi); }

    /// Formats this range as P4 syntax by converting to a little-endian range of bits, and
    /// formatting the result as "[hi:lo]".
    cstring formatAsSlice(int spaceSize) const {
        auto r = toOrder<Endian::Little>(spaceSize);
        auto hi = r.hi;
        auto lo = r.lo;
        if (Unit == RangeUnit::Byte) {
            lo = lo * 8;
            hi = hi * 8 + 7;
        } else {
            BUG_CHECK(Unit == RangeUnit::Bit, "mismatch range units");
        }
        std::stringstream out;
        out << "[" << hi << ":" << lo << "]";
        return cstring(out.str());
    }

    /// The lowest numbered index in the range. For Endian::Network, this is the
    /// most significant bit or byte; for Endian::Little, it's the least
    /// significant.
    int lo;

    /// The highest numbered index in the range. For Endian::Network, this is the
    /// least significant bit or byte; for Endian::Little, it's the most
    /// significant. Because this is a closed range, the range element this
    /// index identifies is included in the range.
    int hi;
};

/** Implements range subtraction (similar to set subtraction), computing the
 * upper and lower ranges that result from @left - @right.  For example,
 * @left - @right = { C.lower, C.upper }:
 *
 *   example   (1)          (2)         (3)         (4)
 *   @left     --------     ----             ----      ----
 *   @right       ---            ----   ----        ----------
 *   C.lower   ---          ----        (empty)     (empty)
 *   C.upper         --     (empty)          ----   (empty)
 *
 * XXX: This could produce a bit vector.
 */
template <RangeUnit Unit, Endian Order>
std::pair<HalfOpenRange<Unit, Order>, HalfOpenRange<Unit, Order>> operator-(
    HalfOpenRange<Unit, Order> left, HalfOpenRange<Unit, Order> right) {
    HalfOpenRange<Unit, Order> empty = {0, 0};
    HalfOpenRange<Unit, Order> intersect = left.intersectWith(right);
    if (intersect.empty())
        return left.lo < right.lo ? std::make_pair(left, empty) : std::make_pair(empty, left);

    HalfOpenRange<Unit, Order> lower =
        left.lo == intersect.lo ? empty : BitRange::FromTo(left.lo, intersect.lo - 1);
    HalfOpenRange<Unit, Order> upper =
        left.hi == intersect.hi ? empty : BitRange::FromTo(intersect.hi, left.hi - 1);

    return {lower, upper};
}

/// @see HalfOpenRange subtraction.
/// @returns a pair of half-open ranges (which may be empty).
template <RangeUnit Unit, Endian Order>
std::pair<HalfOpenRange<Unit, Order>, HalfOpenRange<Unit, Order>> operator-(
    ClosedRange<Unit, Order> left, ClosedRange<Unit, Order> right) {
    return toHalfOpenRange(left) - toHalfOpenRange(right);
}

/// @return a half-open range denoting the same elements as the provided closed
/// range.
template <RangeUnit Unit, Endian Order>
HalfOpenRange<Unit, Order> toHalfOpenRange(ClosedRange<Unit, Order> closedRange) {
    return HalfOpenRange<Unit, Order>(closedRange.lo, closedRange.hi + 1);
}

/// @return a closed range denoting the same elements as the provided half-open
/// range, or std::nullopt if the provided range is empty.
template <RangeUnit Unit, Endian Order>
std::optional<ClosedRange<Unit, Order>> toClosedRange(HalfOpenRange<Unit, Order> halfOpenRange) {
    if (halfOpenRange.empty()) return std::nullopt;
    return ClosedRange<Unit, Order>(halfOpenRange.lo, halfOpenRange.hi - 1);
}

/// Convenience typedefs for closed ranges in bits.
using nw_bitrange = ClosedRange<RangeUnit::Bit, Endian::Network>;
using le_bitrange = ClosedRange<RangeUnit::Bit, Endian::Little>;

/// Convenience typedefs for closed ranges in bytes.
using nw_byterange = ClosedRange<RangeUnit::Byte, Endian::Network>;
using le_byterange = ClosedRange<RangeUnit::Byte, Endian::Little>;

/// Convenience typedefs for half-open ranges in bits.
using nw_bitinterval = HalfOpenRange<RangeUnit::Bit, Endian::Network>;
using le_bitinterval = HalfOpenRange<RangeUnit::Bit, Endian::Little>;

/// Convenience typedefs for half-open ranges in bytes.
using nw_byteinterval = HalfOpenRange<RangeUnit::Byte, Endian::Network>;
using le_byteinterval = HalfOpenRange<RangeUnit::Byte, Endian::Little>;

std::ostream &toStream(std::ostream &out, RangeUnit unit, Endian order, int lo, int hi,
                       bool closed);

template <RangeUnit Unit, Endian Order>
std::ostream &operator<<(std::ostream &out, const HalfOpenRange<Unit, Order> &range) {
    return toStream(out, Unit, Order, range.lo, range.hi, false);
}

template <RangeUnit Unit, Endian Order>
std::ostream &operator<<(std::ostream &out, const ClosedRange<Unit, Order> &range) {
    return toStream(out, Unit, Order, range.lo, range.hi, true);
}

// Hashing specializations
namespace std {
template <RangeUnit Unit, Endian Order>
struct hash<HalfOpenRange<Unit, Order>> {
    std::size_t operator()(const HalfOpenRange<Unit, Order> &r) const {
        return Util::Hash{}(r.lo, r.hi);
    }
};

template <RangeUnit Unit, Endian Order>
struct hash<ClosedRange<Unit, Order>> {
    std::size_t operator()(const ClosedRange<Unit, Order> &r) const {
        return Util::Hash{}(r.lo, r.hi);
    }
};
}  // namespace std

namespace Util {
template <RangeUnit Unit, Endian Order>
struct Hasher<HalfOpenRange<Unit, Order>> {
    size_t operator()(const HalfOpenRange<Unit, Order> &r) const {
        return Util::Hash{}(r.lo, r.hi);
    }
};

template <RangeUnit Unit, Endian Order>
struct Hasher<ClosedRange<Unit, Order>> {
    size_t operator()(const ClosedRange<Unit, Order> &r) const { return Util::Hash{}(r.lo, r.hi); }
};
}  // namespace Util

#endif /* LIB_BITRANGE_H_ */
