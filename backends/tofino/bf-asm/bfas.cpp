/**
 * Copyright (C) 2024 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
 * except in compliance with the License.  You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the
 * License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied.  See the License for the specific language governing permissions
 * and limitations under the License.
 *
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "bfas.h"

#include <unistd.h>

#include <sys/stat.h>

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "backends/tofino/bf-asm/target.h"
#include "backends/tofino/bf-p4c/git_sha_version.h"  // for BF_P4C_GIT_SHA
#include "backends/tofino/bf-p4c/version.h"
#include "constants.h"
#include "lib/indent.h"
#include "misc.h"
#include "parser-tofino-jbay.h"
#include "sections.h"
#include "top_level.h"

#define MAJOR_VERSION 1
#define MINOR_VERSION 0

const std::string SCHEMA_VERSION = CONTEXT_SCHEMA_VERSION;  // NOLINT(runtime/string)

option_t options = {
    .binary = PIPE0,
    .condense_json = true,
    .debug_info = false,
    .disable_egress_latency_padding = false,
    .disable_gfm_parity = true,
    .disable_long_branch = false,
    .disable_power_gating = false,
    .gen_json = false,
    .high_availability_enabled = true,
    .match_compiler = false,
    .multi_parsers = true,  // TODO Remove option after testing
    .partial_input = false,
    .singlewrite = true,
    .stage_dependency_pattern = "",
    .target = NO_TARGET,
    .tof2lab44_workaround = false,
    .version = CONFIG_OLD,
    .werror = false,
    .nowarn = false,
    .log_hashes = false,
    .output_dir = ".",
    .num_stages_override = 0,
    .tof1_egr_parse_depth_checks_disabled = false,
    .fill_noop_slot = nullptr,
};

std::string asmfile_name;                       // NOLINT(runtime/string)
std::string asmfile_dir;                        // NOLINT(runtime/string)
std::string gfm_log_file_name = "mau.gfm.log";  // NOLINT(runtime/string)

std::unique_ptr<std::ostream> gfm_out;

int log_error = 0;
extern char *program_name;

/**
 * @brief Maximum handle offset which can be used for table and parser handles.
 *
 * Selected bits in parser and table handles are dedicated to distinguish handles
 * for different pipes.
 * See comments in bf-asm/parser.h and bf-asm/p4_table.cpp to get more information
 * about format of parser and table handles.
 * Currently 4 bits are dedicated for pipe id.
 */
#define MAX_HANDLE_OFFSET 16

/**
 * @brief Value OR-ed with table and parser handles to create unique handles.
 *
 * See comments in bf-asm/parser.h and bf-asm/p4_table.cpp to get more information
 * about format of parser and table handles.
 */
unsigned unique_table_offset = 0;

BaseAsmParser *asm_parser = nullptr;

// Create target-specific section for parser
void createSingleAsmParser() {
    if (asm_parser != nullptr) {
        return;
    }
    asm_parser = new AsmParser;
}

std::unique_ptr<std::ostream> open_output(const char *name, ...) {
    char namebuf[1024], *p = namebuf, *end = namebuf + sizeof(namebuf);
    va_list args;
    if (!options.output_dir.empty()) p += snprintf(p, end - p, "%s/", options.output_dir.c_str());
    va_start(args, name);
    if (p < end) p += vsnprintf(p, end - p, name, args);
    va_end(args);
    if (p >= end) {
        std::cerr << "File name too long: " << namebuf << "..." << std::endl;
        snprintf(namebuf, sizeof(namebuf), "/dev/null");
    }
    auto rv = std::unique_ptr<std::ostream>(new std::ofstream(namebuf));
    if (!*rv) {
        std::cerr << "Failed to open " << namebuf << " for writing: " << strerror(errno)
                  << std::endl;
    }
    return rv;
}

std::string usage(std::string tfas) {
    std::string u = "usage: ";
    u.append(tfas);
    u.append(" [-l:Mo:gqtvh] file...");
    return u;
}

void output_all() {
    auto targetName = "unknown";
    switch (options.target) {
#define SET_TOP_LEVEL(TARGET)                            \
    case Target::TARGET::tag:                            \
        new TopLevelRegs<Target::TARGET::register_type>; \
        targetName = Target::TARGET::name;               \
        break;
        FOR_ALL_TARGETS(SET_TOP_LEVEL)
        default:
            std::cerr << "No target set" << std::endl;
            error_count++;
            return;
    }
    json::map ctxtJson;
    const time_t now = time(NULL);
    char build_date[1024];
    struct tm lt;
    localtime_r(&now, &lt);
    BUG_CHECK(&lt);
    strftime(build_date, 1024, "%c", &lt);
    ctxtJson["build_date"] = build_date;
    ctxtJson["schema_version"] = SCHEMA_VERSION;
    ctxtJson["compiler_version"] = BF_P4C_VERSION " (" BF_P4C_GIT_SHA ")";
    ctxtJson["target"] = targetName;
    ctxtJson["program_name"] = asmfile_name;
    ctxtJson["learn_quanta"] = json::vector();
    ctxtJson["parser"] = json::map();
    ctxtJson["phv_allocation"] = json::vector();
    ctxtJson["tables"] = json::vector();
    ctxtJson["mau_stage_characteristics"] = json::vector();
    ctxtJson["configuration_cache"] = json::vector();

    Section::output_all(ctxtJson);
    TopLevel::output_all(ctxtJson);

    json::map driver_options;
    driver_options["hash_parity_enabled"] = !options.disable_gfm_parity;
    driver_options["high_availability_enabled"] = options.high_availability_enabled;
    if (options.target == TOFINO)
        driver_options["tof1_egr_parse_depth_checks_disabled"] =
            options.tof1_egr_parse_depth_checks_disabled;
    ctxtJson["driver_options"] = std::move(driver_options);

    auto json_out = open_output("context.json");
    *json_out << &ctxtJson;

    delete TopLevel::all;
}

void check_target_pipes(int pipe_id) {
    if (pipe_id >= 0) {
        if (pipe_id >= MAX_PIPE_COUNT) {
            std::cerr << "Pipe number (" << pipe_id << ") exceeds implementation limit of pipes ("
                      << MAX_PIPE_COUNT << ")." << std::endl;
            error_count++;
        } else if (pipe_id < Target::NUM_PIPES()) {
            options.binary = static_cast<binary_type_t>(PIPE0 + pipe_id);
        } else {
            std::cerr << "Pipe number (" << pipe_id << ") exceeds maximum number of pipes ("
                      << Target::NUM_PIPES() << ") for target " << Target::name() << "."
                      << std::endl;
            error_count++;
        }
    }
}

#define MATCH_TARGET_OPTION(TARGET, OPT)                                         \
    if (!strcasecmp(OPT, Target::TARGET::name)) /* NOLINT(readability/braces) */ \
        options.target = Target::TARGET::tag;                                    \
    else
#define OUTPUT_TARGET(TARGET) << " " << Target::TARGET::name

// Do not build main() when BUILDING_FOR_GTEST.
#ifndef BUILDING_FOR_GTEST
int main(int ac, char **av) {
    int srcfiles = 0;
    const char *firstsrc = 0;
    struct stat st;
    bool asmfile = false;
    bool disable_clog = true;
    int pipe_id = -1;
    extern void register_exit_signals();
    register_exit_signals();
    program_name = av[0];
    std::vector<char *> arguments(av, av + ac);
    static std::set<std::string> valid_noop_fill = {"and",  "or",   "alu_a", "alu_b",
                                                    "minu", "mins", "maxu",  "maxs"};
    if (auto opt = getenv("BFAS_OPTIONS")) {
        int add_at = 1;
        while (auto p = strsep(&opt, " \t\r\n")) {
            if (!*p) continue;
            arguments.insert(arguments.begin() + add_at++, p);
        }
        av = &arguments[0];
        ac = arguments.size();
    }
    for (int i = 1; i < ac; i++) {
        int val, len;
        if (av[i][0] == '-' && av[i][1] == 0) {
            asm_parse_file("<stdin>", stdin);
        } else if (!strcmp(av[i], "--allpipes")) {
            options.binary = FOUR_PIPE;
        } else if (!strcmp(av[i], "--disable-egress-latency-padding")) {
            options.disable_egress_latency_padding = true;
        } else if (!strcmp(av[i], "--log-hashes")) {
            options.log_hashes = true;
        } else if (!strcmp(av[i], "--disable-longbranch")) {
            options.disable_long_branch = true;
        } else if (!strcmp(av[i], "--enable-longbranch")) {
            if (options.target && Target::LONG_BRANCH_TAGS() == 0) {
                error(-1, "target %s does not support --enable-longbranch", Target::name());
                options.disable_long_branch = true;
            } else {
                options.disable_long_branch = false;
            }
        } else if (!strcmp(av[i], "--gen_json")) {
            options.gen_json = true;
            options.binary = NO_BINARY;
        } else if (!strcmp(av[i], "--high_availability_disabled")) {
            options.high_availability_enabled = false;
        } else if (!strcmp(av[i], "--no-condense")) {
            options.condense_json = false;
        } else if (!strcmp(av[i], "--no-bin")) {
            options.binary = NO_BINARY;
        } else if (!strcmp(av[i], "--no-warn")) {
            options.nowarn = true;
        } else if (!strcmp(av[i], "--old_json")) {
            std::cerr << "Old context json is no longer supported" << std::endl;
            error_count++;
        } else if (!strcmp(av[i], "--partial")) {
            options.partial_input = true;
        } else if (sscanf(av[i], "--pipe%d%n", &val, &len) > 0 && !av[i][len] && val >= 0) {
            pipe_id = val;
        } else if (!strcmp(av[i], "--singlepipe")) {
            options.binary = ONE_PIPE;
        } else if (!strcmp(av[i], "--singlewrite")) {
            options.singlewrite = true;
        } else if (!strcmp(av[i], "--multi-parsers")) {
            options.multi_parsers = true;
        } else if (!strcmp(av[i], "--disable-tof2lab44-workaround")) {
            options.tof2lab44_workaround = false;
        } else if (!strcmp(av[i], "--tof2lab44-workaround")) {
            options.tof2lab44_workaround = true;
        } else if (!strcmp(av[i], "--stage_dependency_pattern")) {
            ++i;
            if (!av[i]) {
                std::cerr << "No stage dependency pattern specified " << std::endl;
                error_count++;
                break;
            }
            options.stage_dependency_pattern = av[i];
        } else if (!strcmp(av[i], "--noop-fill-instruction")) {
            ++i;
            if (!av[i] || !valid_noop_fill.count(av[i])) {
                std::cerr << "invalid fill instruction " << av[i] << std::endl;
            } else {
                options.fill_noop_slot = av[i];
            }
        } else if (val = 0, sscanf(av[i], "--noop-fill-instruction=%n", &val), val > 0) {
            if (!valid_noop_fill.count(av[i] + val)) {
                std::cerr << "invalid fill instruction " << (av[i] + val) << std::endl;
            } else {
                options.fill_noop_slot = av[i] + val;
            }
        } else if (sscanf(av[i], "--table-handle-offset%d", &val) > 0 && val >= 0 &&
                   val < MAX_HANDLE_OFFSET) {
            unique_table_offset = val;
        } else if (sscanf(av[i], "--num-stages-override%d", &val) > 0 && val >= 0) {
            options.num_stages_override = val;
        } else if (!strcmp(av[i], "--target")) {
            ++i;
            if (!av[i]) {
                std::cerr << "No target specified '--target <target>'" << std::endl;
                error_count++;
                break;
            }
            if (options.target != NO_TARGET) {
                std::cerr << "Multiple target options" << std::endl;
                error_count++;
                break;
            }
            FOR_ALL_TARGETS(MATCH_TARGET_OPTION, av[i]) {
                std::cerr << "Unknown target " << av[i] << std::endl;
                error_count++;
                std::cerr << "Supported targets:" FOR_ALL_TARGETS(OUTPUT_TARGET) << std::endl;
            }
        } else if (av[i][0] == '-' && av[i][1] == '-') {
            FOR_ALL_TARGETS(MATCH_TARGET_OPTION, av[i] + 2) {
                std::cerr << "Unrecognized option " << av[i] << std::endl;
                error_count++;
            }
        } else if (av[i][0] == '-' || av[i][0] == '+') {
            bool flag = av[i][0] == '+';
            for (char *arg = av[i] + 1; *arg;) switch (*arg++) {
                    case 'a':
                        options.binary = FOUR_PIPE;
                        break;
                    case 'C':
                        options.condense_json = true;
                        break;
                    case 'G':
                        options.gen_json = true;
                        options.binary = NO_BINARY;
                        break;
                    case 'g':
                        options.debug_info = true;
                        break;
                    case 'h':
                        std::cout << usage(av[0]) << std::endl;
                        return 0;
                        break;
                    case 'l':
                        ++i;
                        if (!av[i]) {
                            std::cerr << "No log file specified '-l <log file>'" << std::endl;
                            error_count++;
                            break;
                        }
                        disable_clog = false;
                        if (auto *tmp = new std::ofstream(av[i])) {
                            if (*tmp) {
                                /* FIXME -- tmp leaks, but if we delete it, the log
                                 * redirect fails, and we crash on exit */
                                std::clog.rdbuf(tmp->rdbuf());
                            } else {
                                std::cerr << "Can't open " << av[i] << " for writing" << std::endl;
                                delete tmp;
                            }
                        }
                        break;
                    case 'M':
                        options.match_compiler = true;
                        options.condense_json = false;
                        break;
                    case 'o':
                        ++i;
                        if (!av[i]) {
                            std::cerr << "No output directory specified '-o <output dir>'"
                                      << std::endl;
                            error_count++;
                            break;
                        }
                        if (stat(av[i], &st)) {
                            if (mkdir(av[i], 0777) < 0) {
                                std::cerr << "Can't create output dir " << av[i] << ": "
                                          << strerror(errno) << std::endl;
                                error_count++;
                            }
                        } else if (!S_ISDIR(st.st_mode)) {
                            std::cerr << av[i] << " exists and is not a directory" << std::endl;
                            error_count++;
                        }
                        options.output_dir = av[i];
                        break;
                    case 'p':
                        options.disable_power_gating = true;
                        break;
                    case 'q':
                        std::clog.setstate(std::ios::failbit);
                        break;
                    case 's':
                        options.binary = ONE_PIPE;
                        break;
                    case 'T':
                        disable_clog = false;
                        if (*arg) {
                            Log::addDebugSpec(arg);
                            arg += strlen(arg);
                        } else if (++i < ac) {
                            Log::addDebugSpec(av[i]);
                        }
                        break;
                    case 't':
                        ++i;
                        if (!av[i]) {
                            std::cerr << "No target specified '-t <target>'" << std::endl;
                            error_count++;
                            break;
                        }
                        if (options.target != NO_TARGET) {
                            std::cerr << "Multiple target options" << std::endl;
                            error_count++;
                            break;
                        }
                        FOR_ALL_TARGETS(MATCH_TARGET_OPTION, av[i]) {
                            std::cerr << "Unknown target " << av[i];
                            error_count++;
                        }
                        break;
                    case 'v':
                        disable_clog = false;
                        Log::increaseVerbosity();
                        break;
                    case 'W':
                        if (strcmp(arg, "error"))
                            options.werror = true;
                        else
                            std::cout << "Unknown warning option -W" << arg << std::endl;
                        arg += strlen(arg);
                        break;
                    default:
                        std::cerr << "Unknown option " << (flag ? '+' : '-') << arg[-1]
                                  << std::endl;
                        error_count++;
                }
        } else if (FILE *fp = fopen(av[i], "r")) {
            // asm_parse_file needs to know correct number of stages
            if (options.num_stages_override) {
                Target::OVERRIDE_NUM_MAU_STAGES(options.num_stages_override);
            }

            createSingleAsmParser();

            if (!srcfiles++) firstsrc = av[i];
            error_count += asm_parse_file(av[i], fp);
            if (error_count > 0) return 1;
            fclose(fp);
            asmfile = true;
            asmfile_name = get_filename(av[i]);
            asmfile_dir = get_directory(av[i]);
        } else {
            std::cerr << "Can't read " << av[i] << ": " << strerror(errno) << std::endl;
            error_count++;
        }
    }

    check_target_pipes(pipe_id);

    if (disable_clog) std::clog.setstate(std::ios_base::failbit);
    if (!asmfile) {
        std::cerr << "No assembly file specified" << std::endl;
        error_count++;
    }
    if (error_count > 0) std::cerr << usage(av[0]) << std::endl;

    if (Log::verbosity() > 0) {
        gfm_out = open_output("mau.gfm.log");
    }

    if (error_count == 0 && !options.partial_input) {
        // Check if file has no sections
        no_sections_error_exit();
        // Check if mandatory sections are present in assembly
        bool no_section = false;
        no_section |= no_section_error("deparser");
        no_section |= no_section_error("parser");
        no_section |= no_section_error("phv");
        no_section |= no_section_error("stage");
        if (no_section) exit(1);
    }
    if (error_count == 0) {
        Section::process_all();
    }
    if (error_count == 0) {
        if (srcfiles == 1 && options.output_dir.empty()) {
            if (const char *p = strrchr(firstsrc, '/'))
                options.output_dir = p + 1;
            else if (const char *p = strrchr(firstsrc, '\\'))
                options.output_dir = p + 1;
            else
                options.output_dir = firstsrc;
            if (const char *e = strrchr(&options.output_dir[0], '.'))
                options.output_dir.resize(e - &options.output_dir[0]);
            options.output_dir += ".out";
            if (stat(options.output_dir.c_str(), &st) ? mkdir(options.output_dir.c_str(), 0777)
                                                      : !S_ISDIR(st.st_mode))
                options.output_dir.clear();
        }
        output_all();
    }
    if (log_error > 0) warning(0, "%d config errors in log file", log_error);
    return error_count > 0 || (options.werror && warn_count > 0) ? 1 : 0;
}
#endif /* !BUILDING_FOR_GTEST */

std::string toString(target_t target) {
    switch (target) {
        case TOFINO:
            return "Tofino";
        case TOFINO2:
            return "Tofino2";
        case TOFINO2H:
            return "Tofino2H";
        case TOFINO2U:
            return "Tofino2U";
        case TOFINO2M:
            return "Tofino2M";
        case TOFINO2A0:
            return "Tofino2A0";
        default:
            BUG("Unexpected target value: 0x%x", target);
            return "";
    }
}

std::ostream &operator<<(std::ostream &out, target_t target) { return out << toString(target); }

void no_sections_error_exit() {
    if (Section::no_sections_in_assembly()) {
        std::cerr << "No valid sections found in assembly file" << std::endl;
        exit(1);
    }
}

bool no_section_error(const char *name) {
    if (!Section::section_in_assembly(name)) {
        std::cerr << "No '" << name << "' section found in assembly file" << std::endl;
        return true;
    }
    return false;
}

class Version : public Section {
    Version() : Section("version") {}

    void input(VECTOR(value_t) args, value_t data) {
        if (data.type == tINT || data.type == tVEC) {  // version 1.0.0
            parse_version(data);
        } else if (data.type == tMAP) {  // version 1.0.1
            for (auto &kv : MapIterChecked(data.map, true)) {
                if (kv.key == "version" && (kv.value.type == tVEC || kv.value.type == tINT)) {
                    parse_version(kv.value);
                } else if (kv.key == "run_id" && kv.value.type == tSTR) {
                    _run_id = kv.value.s;
                } else if (kv.key == "compiler") {
                    if (kv.value.type == tSTR) {
                        _compiler = kv.value.s;
                    } else if (kv.value.type == tINT) {
                        _compiler = std::to_string(kv.value.i);
                    } else if (kv.value.type == tVEC) {
                        const char *sep = "";
                        for (auto &el : kv.value.vec) {
                            _compiler += sep;
                            if (el.type == tSTR)
                                _compiler += el.s;
                            else if (el.type == tINT)
                                _compiler += std::to_string(el.i);
                            else
                                error(el.lineno, "can't understand compiler version");
                            sep = ".";
                        }
                    }
                } else if (kv.key == "target") {
                    if (kv.value.type == tSTR) {
                        auto old = options.target;
                        FOR_ALL_TARGETS(MATCH_TARGET_OPTION, kv.value.s) {
                            error(kv.value.lineno, "Unknown target %s", kv.value.s);
                        }
                        if (old != NO_TARGET && old != options.target) {
                            options.target = old;
                            error(kv.value.lineno, "Inconsistent target %s (previously set to %s)",
                                  kv.value.s, Target::name());
                        }
                        createSingleAsmParser();
                    } else {
                        error(kv.value.lineno, "Invalid target %s", value_desc(kv.value));
                    }
                } else {
                    warning(kv.key.lineno, "ignoring unknown item %s in version",
                            value_desc(kv.key));
                }
            }
        } else {
            error(data.lineno, "Invalid version section");
        }
    }

    void output(json::map &ctx_json) {
        if (!_compiler.empty()) ctx_json["compiler_version"] = _compiler;
        ctx_json["run_id"] = _run_id;
    }

 private:
    void parse_version(value_t data) {
        if (data.type == tINT) {
            if (data.i != MAJOR_VERSION)
                error(data.lineno, "Version %" PRId64 " not supported", data.i);
        } else if (data.vec.size >= 2) {
            if (CHECKTYPE(data[0], tINT) && CHECKTYPE(data[1], tINT) &&
                (data[0].i != MAJOR_VERSION || data[1].i > MINOR_VERSION))
                error(data.lineno, "Version %" PRId64 ".%" PRId64 " not supported", data[0].i,
                      data[1].i);
        } else {
            error(data.lineno, "Version not understood");
        }
    }

    std::string _run_id, _compiler;
    static Version singleton_version;
} Version::singleton_version;
