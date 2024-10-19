/**
 * Copyright 2013-2024 Intel Corporation.
 *
 * This software and the related documents are Intel copyrighted materials, and your use of them
 * is governed by the express license under which they were provided to you ("License"). Unless
 * the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
 * or transmit this software or the related documents without Intel's prior written permission.
 *
 * This software and the related documents are provided as is, with no express or implied
 * warranties, other than those that are expressly stated in the License.
 */

#ifndef BF_GTEST_HELPERS_H_
#define BF_GTEST_HELPERS_H_

/** Helper functions and classes for making the creation of gtest easier and their running faster.
 *  See test-bf_getst_helper.cpp for example usage.
 */

#include <initializer_list>
#include <iosfwd>
#include <regex>
#include <string>
#include <vector>

#include <boost/operators.hpp>

#include "lib/compile_context.h"

// Forward declarations.
namespace P4 {
class Visitor;
struct Visitor_Context;
namespace IR {
class P4Program;
}  // namespace IR
namespace IR {
namespace BFN {
class Pipe;
}  // namespace BFN
}  // namespace IR
}  // namespace P4
namespace BFN {
class Backend;
}
class MauAsmOutput;

namespace P4::Test {

namespace Match {

/** The Match namespace brings together low level generic helper functionality.
 *  They play a supportive role to 'TestCode' and others to follow.
 *  End user tests should not need to call 'Match::function' directly - work in progress!
 */

std::string trimWhiteSpace(std::string str);

/// Annotations with nested parentheses are not supported e.g. "@anot(func())"
std::string trimAnnotations(const std::string &str);

/// Helper flags for use by other interfaces.
enum Flag { Raw = 0, TrimWhiteSpace = 1, TrimAnnotations = 2 };
/// Flags may be ORed together.
inline Flag operator|(Flag a, Flag b) {
    return static_cast<Flag>(static_cast<int>(a) | static_cast<int>(b));
}

/** 'convert_to_regex()' allows user to write plain string embedded with chunks of regex.
 *  See std::ECMAScript for details on regex format.
 *  You do not have to escape your regex chunks, but it makes things much cleaner!
 *  Regex chucks placed within `` (back quotes) are not converted, everything else is.
 *  The result is a string suitable for std::regex().
 *  If you require a back quote "`", use its ASCII escape code "\x60".
 *  N.B. The usual escape characters are required for a string literal e.g. '\\' for '\'.
 *           "$slice`(\\d+)`[7:0] ++ $slice`\\1`[7:0]"
 *  or you can use Raw string literals
 *           R"($slice`(\d+)`[7:0] ++ $slice`\1`[7:0])"
 *  The returned string will be an escaped regex literal
 *           R"(\$slice(\d+)\[7:0\] \+\+ \$slice\1\[7:0\])"
 *  In the example above:
 *       `(\d+)` will match 1 or more digits.
 *       `\1` will match what was actually matched by the first regex match.
 *  Thus will match code such as:
 *           $slice42[7:0] ++ $slice42[7:0]
 */
std::string convert_to_regex(const std::string &expr);

/** 'match_basic()' compares 'expr' with 'str', staring at 'str[pos]'.
 *  The search will not include the 'str[n_pos]'character.
 *  Returns either the position in the searched string the match got to. N.B. this may
 *  be (one) beyond the end of 'str'!
 *  Or 'failed'.
 */
constexpr size_t failed = std::string::npos;
size_t match_basic(const std::string &expr, const std::string &str, size_t pos = 0,
                   size_t n_pos = std::string::npos);

/** A 'CheckList' is a series of expressions to match.
 *  The expressions may optionally contain regex expressions - see 'convert_to_regex()'.
 *  When 'TrimWhiteSpace' is used, entries may be separated by zero or more white-space.
 */
typedef std::vector<std::string> CheckList;

/// 'Result' is used to report how a match has proceeded.
struct Result : boost::equality_comparable<Result> {
    bool success;  ///< true if the 'CheckList' was completed.
    size_t pos;    ///< the position in 'str' the match was successful to.
                   ///< N.B. 'pos' may be (one) beyond the end of 'str'!
    size_t count;  ///< tHe number of 'CheckList' item successfully matched.
    /// Match strings
    std::vector<std::string> matches;
    Result(bool success, size_t pos, size_t count) : success(success), pos(pos), count(count) {}
    Result(bool success, size_t pos, size_t count, std::smatch match)
        : success(success), pos(pos), count(count) {
        for (std::size_t n = 0; n < match.size(); ++n) matches.push_back(match[n]);
    }
    friend bool operator==(const Result &l, const Result &r) {
        return l.success == r.success && l.pos == r.pos && l.count == r.count;
    }
};

/** 'match()' is similar to 'match_basic()'.
 *  However it allows a match to be made up of a 'Checklist' of expressions.
 *  It calls 'convert_to_regex()' on the expressions, allowing regex expressions to be used.
 *  See comments regarding 'convert_to_regex()' for more details.
 */
Result match(const CheckList &exprs, const std::string &str, size_t pos = 0,
             size_t n_pos = std::string::npos, Flag flag = Raw);

// 'ends' can be any two differing characters e.g. "AZ".
inline const std::string BraceEnds() { return "{}"; }
inline const std::string ParenEnds() { return "()"; }
inline const std::string SquareEnds() { return "[]"; }
inline const std::string AngleEnds() { return "<>"; }

/// 'find_next_end' finds the closing end, where 'pos' beyond the opening character.
size_t find_next_end(const std::string &str, size_t pos, const std::string &ends);

// 'find_next_block' finds the opening & closing ends, where 'pos' is before the opening character.
std::pair<size_t, size_t> find_next_block(const std::string &str, size_t pos,
                                          const std::string &ends);
/// 'get_ends' returns the appropriate ends pair.
///  viz 'BraceEnds', 'ParenEnds', 'SquareEnds', 'AngleEnds' or an empty string.
std::string get_ends(char opening);

}  // namespace Match

class TestCode {
    // AutoCompileContext adds to the stack a new compilation context for the test to run in.
    AutoCompileContext context;
    const IR::P4Program *program = nullptr;  // Used by frontend and midend passes.
    const IR::BFN::Pipe *pipe = nullptr;     // Used by backend passes.
    BFN::Backend *backend = nullptr;         // Used by extract_asm()
    mutable MauAsmOutput *mauasm = nullptr;  // lazy initialised & used by extract_asm().

    Match::Flag flag = Match::TrimWhiteSpace;  // N.B. common to match() & extract_code().
    std::regex marker;  // Optional search string for the block we are interested in.
    std::string ends;   // How our optional block starts and ends.

    std::string phv_log_file = {};  // path to phv log file

 public:
    const IR::P4Program *get_program() { return program; }
    /// Useful strings.
    static std::string any_to_brace() { return "`([^\\{]*\\{)`"; }  // viz ".*{"
    static std::string empty_state() { return "state start {transition accept;}"; }
    static std::string empty_appy() { return "apply {}"; }

    /** 'min_control_shell' is a minimal program, requiring insertions for:
     *   %0%     Defines for 'struct headers_t' and 'struct local_metadata_t'.
     *   %1%     A control ingress_control block e.g. 'empty_appy()'.
     *  The 'min_control_shell_marker identifies the block within the 'min_control_shell' string.
     */
    static std::string min_control_shell();  // See cpp file for the code.
    static std::string min_control_shell_marker() { return "control testingress" + any_to_brace(); }

    /** 'tofino_shell' is a tofino program, requiring insertions for:
     *   %0%     Defines for 'struct headers_t' and 'struct local_metadata_t'.
     *   %1%     A parser ingress_parser block e.g. 'empty_state()'.
     *   %2%     A control ingress_control block e.g. 'empty_appy()'.
     *   %3%     A control ingress_deparser block e.g. 'empty_appy()'.
     *  The 'tofino_shell_*_marker's identify those blocks within the 'tofino_shell' string.
     */
    static std::string tofino_shell();  // See cpp file for the code.
    static std::string tofino_shell_parser_marker() {
        return "parser ingress_parser" + any_to_brace();
    }
    static std::string tofino_shell_control_marker() {
        return "control ingress_control" + any_to_brace();
    }
    static std::string tofino_shell_deparser_marker() {
        return "control ingress_deparser" + any_to_brace();
    }
    // TODO would a smaller test program (as used in test_bf_gtest_helpers.cpp) be of general value?

    /**  The header file to prefix the 'code' with.
     *  N.B. 98% of the test time can be taken up processing boiler-plate for Front-end tests.
     *       But Mid-end & Back-end test also run faster by minimising unnecessary code.
     *       e.g. preferably use 'Hdr::None', or 'Hdr::TofinoMin', rather than 'Hdr::Tofino1arch'.
     *  1. Get your tests running with 'Hdr::Tofino1arch' & 'tofino_shell()'.
     *  2. Prune the dead code before you finish.
     */
    enum class Hdr {
        None,       // You need to provide the 'package' - see min_control_shell().
        CoreP4,     // The core.p4 package.
        TofinoMin,  // For building an empty tofino_shell() - add only what you need.
        Tofino1arch,
        Tofino2arch,  // The regular header files
        V1model_2018,
        V1model_2020
    };  // The regular header files.

    /** See test_bf_gtest_helpers.cpp for example usage of 'TestCode'.
     *  State the 'header' needed and the 'code' (with optional %N% insertion points).
     *  The %N% entires are repalced by the 'insertion' elements e.g.
     *     TestCode(Hdr::None,                                      // No header required.
     *              R"      control TestIngress<H, M>(inout H hdr, inout M meta);
     *                      package TestPackage<H, M>(TestIngress<H, M> ig);
     *                      %0%     // Definitions.
     *                      control ti(inout Hd headers, inout Md meta){
     *                          %1% // Apply block.
     *                      }
     *                      TestPackage(ti()) main;)",              // Our program.
     *              {"struct Hd{}; struct Md{};", "apply{}"},       // replaces %0% & %1%.
     *              "control ti" + any_to_brace(),                  // The start of the block.
     *              {"-S"});                                        // Commandline options.
     */
    TestCode(Hdr header,        ///< Required header.
             std::string code,  ///< Program, with insertions points.
             const std::initializer_list<std::string> &insertion = {},  ///< Default: none.
             const std::string &blockMarker = "",                       ///< Default: all code.
             /// Default: P4_16, tofino, tna (or tofinoX, tXna when using TofinoXarch header file)
             const std::initializer_list<std::string> &options = {});

    /** See test_bf_gtest_helpers.cpp for example usage of 'TestControlBlock'.
     *  TestControlBlock() is a handy wrapper for testing minimal control block P4 programs.
     *  N.B. 98% of the test time can be taken up processing boiler-plate.
     *       If this wrapper is not quite right, create your own minimal code block!
     *
     * @param defines   Must contain a definition of 'struct Headers'.
     * @param block     The code inserted into 'control testingress(inout Headers headers){%1%}'.
     * @param header    Required header.
     */
    static TestCode TestControlBlock(const std::string &defines, const std::string &block,
                                     Hdr header = Hdr::None) {
        std::initializer_list<std::string> insert = {defines, block};
        return TestCode(header, min_control_shell(), insert, min_control_shell_marker());
    }

    /// Sets the flags to be used by other member functions.
    void flags(Match::Flag f) { flag = f; }

    void set_phv_log_file(std::string path) { phv_log_file = path; }

    /// Runs the pass over either the pipe (if created) else the program.
    bool apply_pass(Visitor &pass, const Visitor_Context *context = nullptr);
    bool apply_pass(Visitor *pass, const Visitor_Context *context = nullptr) {
        return apply_pass(*pass, context);
    }
    /// Runs a preconstructed pass over the pipe or program.
    enum class Pass {
        FullFrontend,          ///< Remove the pipe and run over the program.
        FullMidend,            ///< Remove the pipe and run over the program.
        ConverterToBackend,    ///< Run over the program and create the pipe.
        ThreadLocalInstances,  ///< Run over the pipe.
        FullBackend,           ///< Run over the pipe, creates asm CodeBlocks.
        PhvLogging
    };  ///< Run over the pipe, creates phv.json.
    bool apply_pass(Pass pass);

    /// Runs all the necessary passes to create a backend ready for testing.
    bool CreateBackend() {
        return apply_pass(Pass::FullFrontend) && apply_pass(Pass::FullMidend) &&
               apply_pass(Pass::ConverterToBackend);
    }
    bool CreateBlockThreadLocalInstances() {
        return CreateBackend() && apply_pass(Pass::ThreadLocalInstances);
    }

    /// CodeBlock flag for passing into `match()` and `extract_code()`.
    /// P4Code is always available, xxxAsm code is only availble post `Pass::FullBackend`.
    enum class CodeBlock {
        P4Code,  // P4Code combines with `blockMarker`.
        PhvAsm,
        MauAsm,  // Ingress & Egress.
        HdrAsm,  // Ingress & Egress.
        ParserIAsm,
        DeparserIAsm,  // Ingress.
        ParserEAsm,
        DeparserEAsm
    };  // Egress.

    /// Calls Match::match() on the code block specified by `CodeBlock`.
    Match::Result match(CodeBlock blk_type, const Match::CheckList &exprs) const;
    /// Calls Match::match() on the P4 code block specified by `blockMarker`.
    Match::Result match(const Match::CheckList &exprs) const {
        return match(CodeBlock::P4Code, exprs);
    }

    /// Extract the code block specified by `CodeBlock`
    /// stripping of the initial `pos` characters e.g. `Match::Result.pos`.
    std::string extract_code(CodeBlock blk_type, size_t pos = 0) const;
    /// Extract the P4 code block specified by `blockMarker`.
    /// stripping of the initial `pos` characters e.g. `Match::Result.pos`.
    std::string extract_code(size_t pos = 0) const { return extract_code(CodeBlock::P4Code, pos); }

    /** Get the container for given field
     *
     * @param field  The field to search for
     * @param str    The string in which to search for the field (most likely returned by
     *               `extract_code(CodeBlock::PhvAsm)`
     * @param idx    The index of the (stage X, container) pair to return the conntainer for. This
     *               is _not_ the stage number.
     */
    std::string get_field_container(const std::string &field, const std::string &str,
                                    int idx = 0) const;

    friend std::ostream &operator<<(std::ostream &out, const TestCode &tc) {
        return out << tc.extract_code(CodeBlock::P4Code, 0);
    }

 private:
    std::string extract_p4() const;
    std::string extract_asm(CodeBlock blk_type) const;
};

}  // namespace P4::Test

#endif /* BF_GTEST_HELPERS_H_ */
