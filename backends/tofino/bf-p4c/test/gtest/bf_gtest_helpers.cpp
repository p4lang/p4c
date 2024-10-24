/**
 * Copyright (C) 2024 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License.  You may obtain a copy
 * of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed
 * under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations under the License.
 *
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bf_gtest_helpers.h"

#include <climits>
#include <exception>
#include <regex>
#include <sstream>
#include <utility>

#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/trim_all.hpp>

#include "bf-p4c/arch/arch.h"
#include "bf-p4c/asm.h"
#include "bf-p4c/backend.h"
#include "bf-p4c/bf-p4c-options.h"
#include "bf-p4c/common/bridged_packing.h"
#include "bf-p4c/common/front_end_policy.h"
#include "bf-p4c/common/parse_annotations.h"
#include "bf-p4c/logging/phv_logging.h"
#include "bf-p4c/logging/source_info_logging.h"
#include "bf-p4c/midend.h"
#include "bf-p4c/phv/create_thread_local_instances.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/frontend.h"  // Used by run_p4c_frontend_passes
#include "frontends/p4/toP4/toP4.h"
#include "frontends/p4/typeMap.h"
#include "frontends/parsers/parserDriver.h"
#include "ir/ir.h"
#include "lib/exceptions.h"
#include "lib/sourceCodeBuilder.h"
#include "p4headers.h"

namespace P4::Test {

namespace Match {
std::string trimWhiteSpace(std::string str) {
    boost::algorithm::trim_fill(str, " ");
    return str;
}

std::string trimAnnotations(const std::string &str) {
    // Are nested parentheses legal? If they are count them.
    auto nestedParen = R"(@\w+\([^\)]*\()";
    if (std::regex_search(str, std::regex(nestedParen)))
        throw std::invalid_argument("Annotation with nested parentheses are not supported");
    auto annotation = R"(@\w+(\([^\)]*\))?)";
    return std::regex_replace(str, std::regex(annotation), "");
}

std::string convert_to_regex(const std::string &expr) {
    auto exprLen = expr.length();
    auto stepOverChar = [exprLen](size_t pos) {
        ++pos;
        BUG_CHECK(pos <= exprLen, "pos more than +1 past the end of a string");
        return pos;
    };

    constexpr auto specialChars = "`^$.|?*+(){[\\";  // Backquote or regex special.
    std::string regex;
    size_t pos = 0;
    for (;;) {
        auto p = expr.find_first_of(specialChars, pos);
        regex += expr.substr(pos, p - pos);  // Copy everything up to p.
        if (p == std::string::npos) return regex;
        if (expr[p] == '`') {
            pos = stepOverChar(p);
            p = expr.find_first_of('`', pos);
            // If range contains a "\x60", the regex parser will escape it.
            regex += expr.substr(pos, p - pos);  // Copy everything up to p.
            if (p == std::string::npos) throw std::invalid_argument("Mismatch back quote");
            pos = stepOverChar(p);
        } else if (expr.compare(p, 4, "\\x60") == 0) {
            // We don't want to add a second '\'. Leave it for the regex parser.
            regex += expr[p];  // The "x60" will be copied as normal.
            pos = stepOverChar(p);
        } else {
            // A regex special char that needs escaping.
            regex += '\\';
            regex += expr[p];
            pos = stepOverChar(p);
        }
    }
}

size_t match_basic(const std::string &expr, const std::string &str, const size_t pos,
                   const size_t n_pos) {
    auto strLen = str.length();
    // std::compare does not mind if n_pos > strLen.
    if (pos > strLen || pos > n_pos) return failed;  // Handle bad arguments.
    auto exprLen = expr.length();
    auto next_pos = pos + exprLen;
    if (next_pos > n_pos) return failed;  // Not enough characters to match against.
    if (str.compare(pos, exprLen, expr) != 0) return failed;  // Not an exact match.
    return next_pos;                                          // Maybe one beyond the end of 'str'.
}

Result match(const CheckList &exprs, const std::string &str, size_t pos, size_t n_pos, Flag flag) {
    auto strLen = str.length();
    if (n_pos > strLen) n_pos = strLen;
    if (pos > strLen || pos > n_pos)
        return Result(/* success */ false, /* pos */ failed, /* count */ 0);  // Handle bad 'pos'.

    auto from = str.cbegin() + pos;
    auto to = str.cbegin() + n_pos;

    std::string regex;
    CheckList regex_exprs;  // Keep a record of conversions.
    std::smatch sm;

    // First attempt the whole CheckList.
    for (const auto &expr : exprs) {
        // Trim spaces if present & required.
        std::string e;
        if (flag & TrimWhiteSpace) e += "` *`";
        e += expr;
        regex_exprs.emplace_back(convert_to_regex(e));
        regex += regex_exprs.back();
    }
    if (std::regex_search(from, to, sm, std::regex(regex)) && !sm.prefix().length())
        return Result(/* success */ true, /* pos */ pos + sm[0].length(),
                      /* count */ exprs.size(), /* match */ sm);

    // Re-run to find out where we failed. Linear search is good enough.
    Result res(/* success */ false, pos, /* count */ 0);
    regex = "";
    for (const auto &r_e : regex_exprs) {
        regex += r_e;  // Reuse the saved 'convert_to_regex()' value.
        if (!std::regex_search(from, to, sm, std::regex(regex)) ||
            sm.prefix().length())  // Once we have a prefix, it wont go away.
            return res;            // Found the fail point.
        ++res.count;
        res.pos = pos + sm[0].length();
    }
    BUG("Unreachable code");
}

size_t find_next_end(const std::string &blk, size_t pos, const std::string &ends) {
    if (ends.length() != 2 || ends[0] == ends[1]) throw std::invalid_argument("Bad ends string");
    int count = 1;
    pos = blk.find_first_of(ends, pos);
    while (pos != std::string::npos) {
        if (blk[pos] == ends[0])
            ++count;
        else if (blk[pos] == ends[1])
            if (--count == 0) return pos;
        pos = blk.find_first_of(ends, pos + 1);
    }
    return pos;
}

std::pair<size_t, size_t> find_next_block(const std::string &blk, size_t pos,
                                          const std::string &ends) {
    if (ends.length() != 2 || ends[0] == ends[1]) throw std::invalid_argument("Bad ends string");
    pos = blk.find_first_of(ends[0], pos);
    if (pos == std::string::npos) return std::make_pair(pos, pos);
    return std::make_pair(pos, find_next_end(blk, pos + 1, ends));
}

std::string get_ends(char opening) {
    switch (opening) {
        case '{':
            return BraceEnds();
        case '(':
            return ParenEnds();
        case '[':
            return SquareEnds();
        case '<':
            return AngleEnds();
        default:
            return "";
    }
}

}  // namespace Match

namespace {
char marker_last_char(const std::string &blockMarker) {
    // Find the actual last character of marker e.g. the '{' of the any_to_open_brace.
    size_t len = blockMarker.length();
    while (len > 1 && blockMarker[len - 1] == '`') {
        --len;  // Remove closing `.
        // Check the contents of the back quoted regex.
        if (blockMarker[len - 1] == '`') {
            --len;  // Remove opening `.
        } else {
            // Remove sub-pattern markers etc.
            int count = 0;
            while (len > 1 && blockMarker[len - 2] != '\\' &&  // not escaped.
                   (blockMarker[len - 1] == '(' || blockMarker[len - 1] == ')')) {
                count += (blockMarker[len - 1] == ')') ? 1 : -1;
                if (count < 0) throw std::invalid_argument("Invalid blockMarker parentheses");
                --len;  // Remove sub-pattern.
            }
            if (!count && blockMarker[len - 1] != '`') {
                --len;  // Remove opening `. It was an empty regex.
            } else if (len > 1 && blockMarker[len - 2] != '\\' &&  // not escaped.
                       blockMarker.find_first_of("^$.|?*+{}[]\\", len - 1) == len - 1) {
                // Ends with a special regex char.
                throw std::invalid_argument("marker_last_char must be a literal");
            } else {
                break;  // Found!
            }
        }
    }
    if (len) return blockMarker[len - 1];
    return 0;
}

std::string CoreP4() {
    return R"(
error {
    NoError,           /// No error.
    PacketTooShort,    /// Not enough bits in packet for 'extract'.
    NoMatch,           /// 'select' expression has no matches.
    StackOutOfBounds,  /// Reference to invalid element of a header stack.
    HeaderTooShort,    /// Extracting too many bits into a varbit field.
    ParserTimeout,     /// Parser execution time limit exceeded.
    ParserInvalidArgument  /// Parser operation was called with a value
                           /// not supported by the implementation.
}

extern packet_in {
    void extract<T>(out T hdr);
    void extract<T>(out T variableSizeHeader,
                    in bit<32> variableFieldSizeInBits);
    T lookahead<T>();
    void advance(in bit<32> sizeInBits);
    bit<32> length();
}

extern packet_out {
    void emit<T>(in T hdr);
}

extern void verify(in bool check, in error toSignal);

@noWarn("unused")
action NoAction() {}

match_kind {
    exact,
    ternary,
    lpm
}
    )";
}

std::string TofinoMin() {
    return R"(
    typedef bit<9>  PortId_t;
    typedef bit<16> MulticastGroupId_t;
    typedef bit<5>  QueueId_t;
    typedef bit<3>  MirrorType_t;
    typedef bit<10> MirrorId_t;
    extern packet_in {
        void extract<T>(out T hdr);
        void extract<T>(out T variableSizeHeader, in bit<32> variableFieldSizeInBits);
        T lookahead<T>();
        void advance(in bit<32> sizeInBits);
        bit<32> length();
    }
    extern packet_out {
        void emit<T>(in T hdr);
    }
    // added Mirror extern because DropPacketWithMirrorEngine pass
    // assumes the Mirror extern is defined.
    extern Mirror {
        Mirror();
        Mirror(MirrorType_t mirror_type);
        void emit(in MirrorId_t session_id);
        void emit<T>(in MirrorId_t session_id, in T hdr);
    }
    header ingress_intrinsic_metadata_t {
        bit<1> resubmit_flag;
        bit<1> _pad1;
        bit<2> packet_version;
        bit<3> _pad2;
        PortId_t ingress_port;
        bit<48> ingress_mac_tstamp;
    }
    struct ingress_intrinsic_metadata_for_tm_t {
        PortId_t ucast_egress_port;
        bit<1> bypass_egress;
        bit<1> deflect_on_drop;
        bit<3> ingress_cos;
        QueueId_t qid;
        bit<3> icos_for_copy_to_cpu;
        bit<1> copy_to_cpu;
        bit<2> packet_color;
        bit<1> disable_ucast_cutthru;
        bit<1> enable_mcast_cutthru;
        MulticastGroupId_t mcast_grp_a;
        MulticastGroupId_t mcast_grp_b;
        bit<13> level1_mcast_hash;
        bit<13> level2_mcast_hash;
        bit<16> level1_exclusion_id;
        bit<9> level2_exclusion_id;
        bit<16> rid;
    }
    struct ingress_intrinsic_metadata_from_parser_t {
        bit<48> global_tstamp;
        bit<32> global_ver;
        bit<16> parser_err;
    }
    header egress_intrinsic_metadata_t {
        bit<7> _pad0;
        PortId_t egress_port;
        bit<5> _pad1;
        bit<19> enq_qdepth;
        bit<6> _pad2;
        bit<2> enq_congest_stat;
        bit<14> _pad3;
        bit<18> enq_tstamp;
        bit<5> _pad4;
        bit<19> deq_qdepth;
        bit<6> _pad5;
        bit<2> deq_congest_stat;
        bit<8> app_pool_congest_stat;
        bit<14> _pad6;
        bit<18> deq_timedelta;
        bit<16> egress_rid;
        bit<7> _pad7;
        bit<1> egress_rid_first;
        bit<3> _pad8;
        QueueId_t egress_qid;
        bit<5> _pad9;
        bit<3> egress_cos;
        bit<7> _pad10;
        bit<1> deflection_flag;
        bit<16> pkt_length;
    }
    struct egress_intrinsic_metadata_from_parser_t {
        bit<48> global_tstamp;
        bit<32> global_ver;
        bit<16> parser_err;
    }
    struct ingress_intrinsic_metadata_for_deparser_t {
        bit<3> drop_ctl;
        bit<3> digest_type;
        bit<3> resubmit_type;
        MirrorType_t mirror_type;
    }
    struct egress_intrinsic_metadata_for_deparser_t {
        bit<3> drop_ctl;
        MirrorType_t mirror_type;
        bit<1> coalesce_flush;
        bit<7> coalesce_length;
    }
    struct egress_intrinsic_metadata_for_output_port_t {
        bit<1> capture_tstamp_on_tx;
        bit<1> update_delay_on_tx;
    }
    parser IngressParserT<H, M>(
        packet_in pkt,
        out H hdr,
        out M ig_md,
        @optional out ingress_intrinsic_metadata_t ig_intr_md,
        @optional out ingress_intrinsic_metadata_for_tm_t ig_intr_md_for_tm,
        @optional out ingress_intrinsic_metadata_from_parser_t ig_intr_md_from_prsr);
    parser EgressParserT<H, M>(
        packet_in pkt,
        out H hdr,
        out M eg_md,
        @optional out egress_intrinsic_metadata_t eg_intr_md,
        @optional out egress_intrinsic_metadata_from_parser_t eg_intr_md_from_prsr);
    control IngressT<H, M>(
        inout H hdr,
        inout M ig_md,
        @optional in ingress_intrinsic_metadata_t ig_intr_md,
        @optional in ingress_intrinsic_metadata_from_parser_t ig_intr_md_from_prsr,
        @optional inout ingress_intrinsic_metadata_for_deparser_t ig_intr_md_for_dprsr,
        @optional inout ingress_intrinsic_metadata_for_tm_t ig_intr_md_for_tm);
    control EgressT<H, M>(
        inout H hdr,
        inout M eg_md,
        @optional in egress_intrinsic_metadata_t eg_intr_md,
        @optional in egress_intrinsic_metadata_from_parser_t eg_intr_md_from_prsr,
        @optional inout egress_intrinsic_metadata_for_deparser_t eg_intr_md_for_dprsr,
        @optional inout egress_intrinsic_metadata_for_output_port_t eg_intr_md_for_oport);
    control IngressDeparserT<H, M>(
        packet_out pkt,
        inout H hdr,
        in M metadata,
        @optional in ingress_intrinsic_metadata_for_deparser_t ig_intr_md_for_dprsr,
        @optional in ingress_intrinsic_metadata_t ig_intr_md);
    control EgressDeparserT<H, M>(
        packet_out pkt,
        inout H hdr,
        in M metadata,
        @optional in egress_intrinsic_metadata_for_deparser_t eg_intr_md_for_dprsr,
        @optional in egress_intrinsic_metadata_t eg_intr_md,
        @optional in egress_intrinsic_metadata_from_parser_t eg_intr_md_from_prsr);
    package Pipeline<IH, IM, EH, EM>(
        IngressParserT<IH, IM> ingress_parser,
        IngressT<IH, IM> ingress,
        IngressDeparserT<IH, IM> ingress_deparser,
        EgressParserT<EH, EM> egress_parser,
        EgressT<EH, EM> egress,
        EgressDeparserT<EH, EM> egress_deparser);
    @pkginfo(arch="TNA", version="0.0.0")
    package Switch<IH0, IM0, EH0, EM0>(
        Pipeline<IH0, IM0, EH0, EM0> pipe);
    )";
}

}  // namespace

TestCode::TestCode(Hdr header, std::string code,
                   const std::initializer_list<std::string> &insertion,
                   const std::string &blockMarker,
                   const std::initializer_list<std::string> &options)
    : context(new BFNContext()) {
    // Set up default options.
    auto &o = BackendOptions();
    o.langVersion = CompilerOptions::FrontendVersion::P4_16;
    switch (header) {
        default:
            o.target = "tofino"_cs;
            o.arch = "tna"_cs;
            // Disable the min depth limit by default as we typically don't want to add parser
            // padding in tests. Set to false if needed.
            o.disable_parse_min_depth_limit = true;
            break;
        case Hdr::Tofino2arch:
            o.target = "tofino2"_cs;
            o.arch = "t2na"_cs;
            break;
    }

    std::vector<char *> argv;
    // We must have at least one command-line option viz the name.
    static const char *testcode = "TestCode";
    argv.push_back(const_cast<char *>(testcode));
    // Add the input options.
    for (const auto &arg : options) argv.push_back(const_cast<char *>(arg.data()));
    argv.push_back(nullptr);
    o.process(argv.size() - 1, argv.data());
    Device::init(o.target);
    BFN::Architecture::init(o.arch);

    std::stringstream source;
    switch (header) {
        case Hdr::None:
            break;
        case Hdr::CoreP4:
            source << CoreP4();
            break;
        case Hdr::TofinoMin:
            source << TofinoMin();
            break;
        case Hdr::Tofino1arch:
            source << Test::p4headers().tofino1arch_p4;
            break;
        case Hdr::Tofino2arch:
            source << Test::p4headers().tofino2arch_p4;
            break;
        case Hdr::V1model_2018:
            source << Test::p4headers().v1model_2018_p4;
            break;
        case Hdr::V1model_2020:
            source << Test::p4headers().v1model_2020_p4;
            break;
    }

    // Replace embedded tokens.
    int i = 0;
    for (auto insert : insertion) {
        std::ostringstream oss;
        oss << '%' << i++ << '%';
        boost::replace_all(code, oss.str(), insert);
    }
    source << code;

    program = P4::P4ParserDriver::parse(source, testcode);
    if (::errorCount() || !program) throw std::invalid_argument("TestCode built a bad program.");

    if (!blockMarker.empty()) {
        marker = std::regex(Match::convert_to_regex(blockMarker));
        ends = Match::get_ends(marker_last_char(blockMarker));
        if (ends.empty())
            throw std::invalid_argument("blockMarker's last char is not a Match::get_ends()");
    }
}

std::string TestCode::min_control_shell() {
    return R"(
    %0%                                                 // Defines 'struct Headers'
    control TestIngress<H>(inout H hdr);
    @pkginfo(arch="FOO", version="0.0.0")
    package Switch<H>(TestIngress<H> ig);
    control testingress(inout Headers headers) {%1%}    // The control block
    Switch(testingress()) main;)";
}

std::string TestCode::tofino_shell() {
    return R"(
    %0% // Define struct headers_t{}; struct local_metadata_t{}
    parser ingress_parser(packet_in packet, out headers_t hdr,
                          out local_metadata_t ig_md, out ingress_intrinsic_metadata_t ig_intr_md)
        {%1%}
    control ingress_control(inout headers_t hdr, inout local_metadata_t ig_md,
                            in ingress_intrinsic_metadata_t ig_intr_md,
                            in ingress_intrinsic_metadata_from_parser_t ig_prsr_md,
                            inout ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md,
                            inout ingress_intrinsic_metadata_for_tm_t ig_tm_md)
        {%2%}
    control ingress_deparser(packet_out packet, inout headers_t hdr, in local_metadata_t ig_md,
                             in ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md)
        {%3%}
    parser egress_parser(packet_in packet, out headers_t hdr, out local_metadata_t eg_md,
                         out egress_intrinsic_metadata_t eg_intr_md)
        {state start {transition accept;}}
    control egress_control(inout headers_t hdr, inout local_metadata_t eg_md,
                           in egress_intrinsic_metadata_t eg_intr_md,
                           in egress_intrinsic_metadata_from_parser_t eg_intr_md_from_prsr,
                           inout egress_intrinsic_metadata_for_deparser_t eg_intr_md_for_dprsr,
                           inout egress_intrinsic_metadata_for_output_port_t eg_intr_md_for_oport)
        {apply{}}
    control egress_deparser(packet_out packet, inout headers_t hdr, in local_metadata_t eg_md,
                            in egress_intrinsic_metadata_for_deparser_t eg_intr_md_for_dprsr)
        {apply{}}
    Pipeline(ingress_parser(),
             ingress_control(),
             ingress_deparser(),
             egress_parser(),
             egress_control(),
             egress_deparser()) pipeline;
    Switch(pipeline) main;)";
}

bool TestCode::apply_pass(Visitor &pass, const Visitor_Context *context) {
    auto before = ::errorCount();
    if (pipe) {
        pipe = pipe->apply(pass, context);
        if (!pipe) std::cerr << "apply_pass to pipe failed\n";
    } else {
        program = program->apply(pass, context);
        if (!program) std::cerr << "apply_pass to program failed\n";
    }
    return ::errorCount() == before;
}

bool TestCode::apply_pass(Pass pass) {
    constexpr bool skip_side_effect_ordering = true;
    auto options = BackendOptions();
    switch (pass) {
        case Pass::FullFrontend: {
            pipe = nullptr;
            backend = nullptr;
            mauasm = nullptr;
            auto before = ::errorCount();
            // The 'frontendPasses' are encapsulated in a run method, so we have to call that.
            auto parseAnnotations = BFN::ParseAnnotations();
            auto policy = BFN::FrontEndPolicy(&parseAnnotations, skip_side_effect_ordering);
            program = P4::FrontEnd(&policy).run(options, program);
            // program->apply(*new BFN::FindArchitecture());
            return ::errorCount() == before;
        }

        case Pass::FullMidend: {
            pipe = nullptr;
            backend = nullptr;
            mauasm = nullptr;
            BFN::MidEnd midend{options};
            program->apply(*new BFN::FindArchitecture());
            return apply_pass(midend);
        }

        case Pass::ConverterToBackend: {
            pipe = nullptr;
            backend = nullptr;
            mauasm = nullptr;
            ordered_map<cstring, const IR::Type_StructLike *> emptyRepackedMap;
            P4::ReferenceMap emptyRefMap;
            CollectSourceInfoLogging emptySrcInfoLog(emptyRefMap);
            SubstitutePackedHeaders extractPipes(options, emptyRepackedMap, emptySrcInfoLog);
            if (!apply_pass(extractPipes) || !extractPipes.pipe.size()) return false;
            pipe = extractPipes.pipe[0];
            return pipe != nullptr;
        }

        case Pass::ThreadLocalInstances: {
            if (!pipe) throw std::invalid_argument("ConverterToBackend must be run first");
            backend = nullptr;
            mauasm = nullptr;
            CreateThreadLocalInstances ctli;
            return apply_pass(ctli);
        }

        case Pass::FullBackend: {
            if (!pipe) throw std::invalid_argument("ConverterToBackend must be run first");
            mauasm = nullptr;
            static int pipe_id = INT_MAX;  // TableSummary requires a unique pipe ID.
            backend = new BFN::Backend{options, pipe_id--};
            return apply_pass(backend);
        }

        case Pass::PhvLogging: {
            if (!backend) throw std::invalid_argument("FullBackend must be run first");
            if (phv_log_file.empty())
                throw std::invalid_argument("Path to phv log file must be set first");
            return apply_pass(new PhvLogging(
                phv_log_file.c_str(), backend->get_phv(), backend->get_clot(),
                *(backend->get_phv_logging()), *(backend->get_phv_logging_defuse_info()),
                backend->get_table_alloc(), backend->get_tbl_summary()));
        }
    }
    return false;
}

Match::Result TestCode::match(CodeBlock blk_type, const Match::CheckList &exprs) const {
    return Match::match(exprs, extract_code(blk_type), 0, std::string::npos, flag);
}

std::string TestCode::extract_p4() const {
    // Fetch the entire program.
    Util::SourceCodeBuilder builder;
    auto before = ::errorCount();
    auto pass = P4::ToP4(builder, false);
    program->apply(pass);
    if (::errorCount() != before) return "Invalid Program";
    auto blk = builder.toString();

    // Trim out the block - the block may move location.
    std::smatch sm;
    if (!ends.empty() && std::regex_search(blk, sm, marker)) {
        size_t start = sm.prefix().length() + sm[0].length();
        size_t end = Match::find_next_end(blk, start, ends);
        if (end != std::string::npos) --end;
        blk = blk.substr(start, end - start);
    }
    return blk;
}

std::string TestCode::extract_asm(CodeBlock blk_type) const {
    BUG_CHECK(blk_type != CodeBlock::P4Code, "Handled by extract_p4() not here!");
    if (!backend) throw std::invalid_argument("FullBackend must be run first");

    // We lazy apply the MauAsmOutput pass, hence we make the variable mutable.
    // TODO do we need to apply this pass before we can extract a non-Mau CodeBlock?
    if (!mauasm) {
        auto before = ::errorCount();
        mauasm = new Tofino::MauAsmOutput{backend->get_phv(), pipe, backend->get_nxt_tbl(),
                                          backend->get_power_and_mpr(), BackendOptions()};
        pipe->apply(*mauasm);
        if (::errorCount() != before) return "Invalid MauAsmOutput";
    }

    std::ostringstream oss;
    switch (blk_type) {
        case CodeBlock::P4Code:
            BUG("Unreachable code");
        case CodeBlock::PhvAsm:
            oss << PhvAsmOutput(backend->get_phv(), backend->get_defuse(),
                                backend->get_tbl_summary(), backend->get_live_range_report(),
                                pipe->ghost_thread.ghost_mau);
            break;
        case CodeBlock::MauAsm:
            oss << *mauasm << "\n";
            break;
        case CodeBlock::HdrAsm:
            BUG("CodeBlock::HdrAsm is not supported disabled");
            break;
        case CodeBlock::ParserIAsm:
            oss << ParserAsmOutput(pipe, backend->get_phv(), backend->get_clot(), INGRESS);
            break;
        case CodeBlock::DeparserIAsm:
            oss << DeparserAsmOutput(pipe, backend->get_phv(), backend->get_clot(), INGRESS);
            break;
        case CodeBlock::ParserEAsm:
            oss << ParserAsmOutput(pipe, backend->get_phv(), backend->get_clot(), EGRESS);
            break;
        case CodeBlock::DeparserEAsm:
            oss << DeparserAsmOutput(pipe, backend->get_phv(), backend->get_clot(), EGRESS);
            break;
    }
    return oss.str();
}

std::string TestCode::extract_code(CodeBlock blk_type, size_t pos) const {
    auto blk = (blk_type == CodeBlock::P4Code) ? extract_p4() : extract_asm(blk_type);

    // Apply any requested options.
    if (flag & Match::TrimAnnotations) blk = Match::trimAnnotations(blk);
    if (flag & Match::TrimWhiteSpace) blk = Match::trimWhiteSpace(blk);
    return blk.substr(pos);
}

std::string TestCode::get_field_container(const std::string &field, const std::string &str,
                                          int idx) const {
    std::smatch sm;

    if (idx < 0) return "";

    // Try simple "field: container" format first
    if (idx == 0) {
        std::string regex_str = R"((\b|\s))" + Match::convert_to_regex(field) + R"(: (\w+))";
        std::regex field_regex = std::regex(regex_str);
        if (std::regex_search(str, sm, field_regex)) {
            return *std::next(sm.begin(), 2);
        }
    }

    // Format: "stage STAGE_NUM: CONTAINER" / "stage STAGE_LO..STAGE_HI: CONTAINER"
    // Step 1: match on the field plus all of its stages
    std::string regex_str =
        R"((\b|\s))" + Match::convert_to_regex(field) +
        R"(: \{\s*((stage \d+(\.\.\d+)?: (\w+)(\(\d+(\.\.\d+)?\))?(,\s*)?)+)\s*\})";
    std::regex field_regex = std::regex(regex_str);
    if (std::regex_search(str, sm, field_regex)) {
        // Step 2: iterate over the individual stage pairs until we find the indicated index
        std::string stage_str = *std::next(sm.begin(), 2);
        std::string regex_str = R"(stage \d+(\.\.\d+)?: (\w+)(\(\d+(\.\.\d+)?\))?)";
        std::regex field_regex = std::regex(regex_str);
        while (std::regex_search(stage_str, sm, field_regex)) {
            if (idx-- == 0) return sm[2];
            stage_str = sm.suffix();
        }
    }

    return "";
}

}  // namespace P4::Test
