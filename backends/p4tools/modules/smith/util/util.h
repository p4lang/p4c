#ifndef BACKENDS_P4TOOLS_MODULES_SMITH_UTIL_UTIL_H_
#define BACKENDS_P4TOOLS_MODULES_SMITH_UTIL_UTIL_H_

#include <cstddef>
#include <string>

namespace P4Tools::P4Smith {

#define INTEGER_WIDTH 32

/// These are hardcoded initialization names.
#define SYS_HDR_NAME "Headers"
#define ETH_HEADER_T "ethernet_t"
#define ETH_HDR "eth_hdr"

/// @returns a randomly generated string.
/// If we can, return a word from a 10,000 word wordlist,
/// if not, generate a random string and return it.
/// @param len : Ignored when choosing from the wordlist.
std::string getRandomString(size_t len);

}  // namespace P4Tools::P4Smith

#endif /* BACKENDS_P4TOOLS_MODULES_SMITH_UTIL_UTIL_H_ */
