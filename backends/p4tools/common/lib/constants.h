#ifndef BACKENDS_P4TOOLS_COMMON_LIB_CONSTANTS_H_
#define BACKENDS_P4TOOLS_COMMON_LIB_CONSTANTS_H_

namespace P4Tools {

class P4Constants {
 public:
    // Parser error codes, copied from core.p4.
    /// No error.
    static constexpr int NO_ERROR = 0x0000;
    /// Not enough bits in packet for 'extract'.
    static constexpr int PARSER_ERROR_PACKET_TOO_SHORT = 0x0001;
    /// 'select' expression has no matches
    static constexpr int PARSER_ERROR_NO_MATCH = 0x0002;
    /// Reference to invalid element of a header stack.
    static constexpr int PARSER_ERROR_STACK_OUT_OF_BOUNDS = 0x0003;
    /// Extracting too many bits into a varbit field.
    static constexpr int PARSER_ERROR_HEADER_TOO_SHORT = 0x0004;
    /// Parser execution time limit exceeded.
    static constexpr int PARSER_ERROR_TIMEOUT = 0x005;
    /// Parser operation was called with a value
    /// not supported by the implementation.
    static constexpr int PARSER_ERROR_INVALID_ARGUMENT = 0x0020;
    /// Match bits exactly.
    static constexpr const char *MATCH_KIND_EXACT = "exact";
    /// Ternary match, using a mask.
    static constexpr const char *MATCH_KIND_TERNARY = "ternary";
    /// Longest-prefix match.
    static constexpr const char *MATCH_KIND_LPM = "lpm";
};

}  // namespace P4Tools

#endif /* BACKENDS_P4TOOLS_COMMON_LIB_CONSTANTS_H_ */
