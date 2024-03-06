#ifndef BACKENDS_P4TOOLS_MODULES_SMITH_UTIL_UTIL_H_
#define BACKENDS_P4TOOLS_MODULES_SMITH_UTIL_UTIL_H_

#include <string>
#include <vector>

#include "backends/p4tools/modules/smith/common/probabilities.h"
#include "ir/ir.h"

#define INTEGER_WIDTH 32

/// These are hardcoded initialization names.
#define SYS_HDR_NAME "Headers"
#define ETH_HEADER_T "ethernet_t"
#define ETH_HDR "eth_hdr"

namespace P4Tools {

namespace P4Smith {

/// @returns a randomly generated string.
/// If we can, return a word from a 10,000 word wordlist,
/// if not, generate a random string and return it.
/// @param len : Ignored when choosing from the wordlist.
std::string randStr(size_t len);

/// Set the seed for the random number generater to the given.
/// @param: seed.
void setSeed(int64_t seed);

/// @returns a random integer based on the percent vector.
int64_t randInt(const std::vector<int64_t> &percent);

/// @returns a random integer between min and max.
int64_t getRndInt(int64_t min, int64_t max);

/// This is a big_int version of getRndInt.
big_int getRndBigInt(big_int min, big_int max);

}  // namespace P4Smith

}  // namespace P4Tools

#endif /* BACKENDS_P4TOOLS_MODULES_SMITH_UTIL_UTIL_H_ */
