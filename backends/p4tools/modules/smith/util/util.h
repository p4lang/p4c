#ifndef BACKENDS_P4TOOLS_MODULES_SMITH_UTIL_UTIL_H_
#define BACKENDS_P4TOOLS_MODULES_SMITH_UTIL_UTIL_H_

#include <string>

#include <boost/random/uniform_int_distribution.hpp>

#include "backends/p4tools/common/lib/util.h"
#include "lib/cstring.h"

namespace P4Tools::P4Smith {

static constexpr int INTEGER_WIDTH(32);

/// The maximum bitwidth that the target supports.
static constexpr int MAX_BITWIDTH(128);

/// These are hardcoded initialization names.
static const cstring SYS_HDR_NAME("Headers");
static const cstring ETH_HEADER_T("ethernet_t");
static const cstring ETH_HDR("eth_hdr");

/// @returns a randomly generated string.
/// If we can, return a word from a 10,000 word wordlist,
/// if not, generate a random string and return it.
/// @param len : Ignored when choosing from the wordlist.
std::string getRandomString(size_t len);

class SmithUtils : public Utils {
 private:
    /// The random generator of this project. It is initialized with the input seed.
    static boost::random::mt19937 rng;

 public:
    /// @returns a random integer in the range [min, max].
    /// This is a template function that supports different integer types.
    template <typename GenericIntType>
    static GenericIntType getRandInt(GenericIntType min, GenericIntType max) {
        static_assert(std::is_integral<GenericIntType>::value,
                      "GenericIntType must be an integral type.");
        static_assert(sizeof(GenericIntType) * 8 <= MAX_BITWIDTH,
                      "GenericIntType must be less than or equal to MAX_BITWIDTH.");

        boost::random::uniform_int_distribution<GenericIntType> distribution(min, max);
        return distribution(rng);
    }
};

}  // namespace P4Tools::P4Smith

#endif /* BACKENDS_P4TOOLS_MODULES_SMITH_UTIL_UTIL_H_ */
