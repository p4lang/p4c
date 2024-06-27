#ifndef BACKENDS_P4TOOLS_MODULES_SMITH_UTIL_UTIL_H_
#define BACKENDS_P4TOOLS_MODULES_SMITH_UTIL_UTIL_H_

#include <string>

#include "lib/cstring.h"

namespace P4Tools::P4Smith {

static constexpr int INTEGER_WIDTH(32);

/// These are hardcoded initialization names.
static const cstring SYS_HDR_NAME("Headers");
static const cstring ETH_HEADER_T("ethernet_t");
static const cstring ETH_HDR("eth_hdr");

/// @returns a randomly generated string.
/// If we can, return a word from a 10,000 word wordlist,
/// if not, generate a random string and return it.
/// @param len : Ignored when choosing from the wordlist.
std::string getRandomString(size_t len);

/// @brief The following functions serve to generate random but well-formed for-loops.
/// @return Each of them returns a string representation of each part of a for-loop.
/// In tandem, they can be used to generate a complete for-loop by calling `generateForLoop()`.
std::string generateLoopControlVariable();
std::string generateLoopInitialization(const std::string &var);
std::string generateLoopCondition(const std::string &var);
std::string generateLoopUpdate(const std::string &var);
std::string generateLoopBody(const std::string &var);
std::string generateLoopBody(const std::string &var1, const std::string &var2);
template <typename... Args>
std::string generateLoopBody(const std::string &var1, const std::string &var2, const Args&... args);
std::string generateTrivialForLoop();
std::string generateHeaderStackForLoop();
std::string generateMultipleInitUpdateForLoop();
std::string generateRangeBasedForLoop(const std::string &start, const std::string &end);
std::string generateForLoop();
}  // namespace P4Tools::P4Smith

#endif /* BACKENDS_P4TOOLS_MODULES_SMITH_UTIL_UTIL_H_ */
