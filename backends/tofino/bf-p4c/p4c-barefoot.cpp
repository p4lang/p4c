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

/**
 * \defgroup bf_p4c Overview of bf-p4c
 * \brief Overview of passes performed by the bf-p4c binary.
 *
 * The compiler goes through this sequence of high-level passes:
 * 1. Frontend
 *   * Parses input P4 file
 *   * Creates IR
 *   * See bf-p4c/frontend.h, bf-p4c/frontend.cpp, and p4c/frontends/
 * 2. Generate P4 Runtime
 *   * Creates the Barefoot runtime JSON file
 *   * Currently also:
 *     * replaces typedefs (P4::EliminateTypedef) – this is also done in midend
 *     * rewrites action selectors to newer syntax (BFN::RewriteActionSelector)
 *   * See bf-p4c/control-plane/p4runtime.h, bf-p4c/control-plane/p4runtime.cpp,
 *     p4c/control-plane/p4RuntimeSerializer.h, and p4c/control-plane/p4RuntimeSerializer.cpp
 * 3. \ref midend
 * 4. \ref post_midend
 *   * Bridge Packing
 *     * Flexible header repack (bridged metadata egress->ingress)
 *   * Substitute Packed Headers
 *     * Transforms the IR towards backend IR (vector of pipes, no longer able to be type-checked)
 *     * Replaces flexible type definition with packed version
 *   * See bf-p4c/arch/bridge.h and bf-p4c/arch/bridge.cpp
 * 5. Source Info Logging
 *   * Creates a JSON file with source info for P4I
 *   * This is done via information that were collected in different parts of the compiler
 *     by CollectSourceInfoLogging
 *   * See bf-p4c/logging/source_info_logging.h and bf-p4c/logging/source_info_logging.cpp
 * 6. Generate graphs
 *   * Essentially a backend for generating graphs of programs
 *   * See p4c/backends/graphs/
 * 7. \ref backend
 */

/**
 * \defgroup post_midend Post-midend
 * \brief Overview of post-midend passes
 *
 * Post-midend passes follow midend; they adjust packing of bridged and fixed-size headers
 * and convert midend IR towards backend IR.
 */

// The following comments of the namespaces are here to make sure they are present in Doxygen.
/**
 * @namespace BFN
 * @brief The namespace encapsulating Barefoot/Intel-specific stuff
 */
/**
 * @namespace IR
 * @brief The namespace encapsulating %IR node classes
 */
/**
 * @namespace PHV
 * @brief The namespace encapsulating PHV-related stuff
 */
/**
 * @namespace Test
 * @brief The namespace encapsulating test-related stuff
 */

//  All C includes should come before the first C++ include, according to one of our Git hooks.
#include <libgen.h>
#include <unistd.h>

#include <sys/stat.h>

#include <climits>
#include <csignal>
#include <cstdio>
#include <iostream>
#include <string>

#include "asm.h"
#include "backend.h"
#include "backends/graphs/controls.h"
#include "backends/graphs/graph_visitor.h"
#include "bf-p4c/backend.h"
#include "bf-p4c/common/bridged_packing.h"
#include "bf-p4c/common/pragma/collect_global_pragma.h"
#include "bf-p4c/control-plane/runtime.h"
#include "bf-p4c/frontend.h"
#include "bf-p4c/lib/error_type.h"
#include "bf-p4c/logging/collect_diagnostic_checks.h"
#include "bf-p4c/logging/event_logger.h"
#include "bf-p4c/logging/filelog.h"
#include "bf-p4c/logging/phv_logging.h"
#include "bf-p4c/logging/resources.h"
#include "bf-p4c/logging/source_info_logging.h"
#include "bf-p4c/mau/dynhash.h"
#include "bf-p4c/mau/table_flow_graph.h"
#include "bf-p4c/midend/type_checker.h"
#include "bf-p4c/parde/parser_header_sequences.h"
#include "common/extract_maupipe.h"
#include "common/run_id.h"
#include "device.h"
#include "frontends/common/constantFolding.h"
#include "frontends/p4-14/header_type.h"
#include "frontends/p4-14/typecheck.h"
#include "frontends/p4/createBuiltins.h"
#include "frontends/p4/validateParsedProgram.h"
#include "ir/dbprint.h"
#include "lib/compile_context.h"
#include "lib/crash.h"
#include "lib/exceptions.h"
#include "lib/gc.h"
#include "lib/log.h"
#include "lib/nullstream.h"
#include "logging/manifest.h"
#include "midend.h"
#include "version.h"

#if !defined(BAREFOOT_INTERNAL) || defined(RELEASE_BUILD)
// Catch all exceptions in production or release environment
#define BFP4C_CATCH_EXCEPTIONS 1
#endif

static void log_dump(const IR::Node *node, const char *head) {
    if (!node || !LOGGING(1)) return;
    if (head)
        std::cout << '+' << std::setw(strlen(head) + 6) << std::setfill('-') << "+\n| " << head
                  << " |\n"
                  << '+' << std::setw(strlen(head) + 3) << "+" << std::endl
                  << std::setfill(' ');
    if (LOGGING(2))
        dump(node);
    else
        std::cout << *node << std::endl;
}

// This structure is sensitive to whether the pass is called for a successful
// compilation or a failed compilation. A successful compilation will output the .bfa
// file and a json file for resources. It is unlikely that the ASM output will generate
// an exception, but if it does, it should be caught by main (in p4c-barefoot.cpp).  If
// it is called with a failed pass, we should still apply the BFN::Visualization pass
// to collect the table resource usages, but we only output the resources file.
//
class GenerateOutputs : public PassManager {
 private:
    const BFN_Options &_options;
    int _pipeId;
    bool _success;
    std::string _outputDir;

    BFN::DynamicHashJson _dynhash;
    const Util::JsonObject &_primitives;
    const Util::JsonObject &_depgraph;

    /// Output a skeleton context.json in case compilation fails.
    /// It is required by all our tools. If the assembler can get far enough, it will overwrite it.
    void outputContext() {
        cstring ctxtFileName = _outputDir + "/context.json"_cs;
        std::ofstream ctxtFile(ctxtFileName);
        rapidjson::StringBuffer sb;
        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(sb);
        writer.StartObject();
        const time_t now = time(NULL);
        char build_date[1024];
        strftime(build_date, 1024, "%c", localtime(&now));
        writer.Key("build_date");
        writer.String(build_date);
        writer.Key("program_name");
        writer.String(std::string(_options.programName + ".p4").c_str());
        writer.Key("run_id");
        writer.String(RunId::getId().c_str());
        writer.Key("schema_version");
        writer.String(CONTEXT_SCHEMA_VERSION);
        writer.Key("compiler_version");
        writer.String(BF_P4C_VERSION);
        writer.Key("target");
        writer.String(std::string(_options.target).c_str());
        writer.Key("tables");
        writer.StartArray();
        writer.EndArray();
        writer.Key("phv_allocation");
        writer.StartArray();
        writer.EndArray();
        writer.Key("parser");
        writer.StartObject();
        writer.Key("ingress");
        writer.StartArray();
        writer.EndArray();
        writer.Key("egress");
        writer.StartArray();
        writer.EndArray();
        writer.EndObject();
        writer.Key("learn_quanta");
        writer.StartArray();
        writer.EndArray();
        writer.Key("mau_stage_characteristics");
        writer.StartArray();
        writer.EndArray();
        writer.Key("dynamic_hash_calculations");
        writer.StartArray();
        writer.EndArray();
        writer.Key("configuration_cache");
        writer.StartArray();
        writer.EndArray();
        writer.Key("driver_options");
        writer.StartObject();
        writer.Key("hash_parity_enabled");
        writer.Bool(false);
        writer.EndObject();
        writer.EndObject();  // end of context
        ctxtFile << sb.GetString();
        ctxtFile.flush();
        ctxtFile.close();
        auto contextDir = BFNContext::get()
                              .getOutputDirectory(""_cs, _pipeId)
                              .substr(_options.outputDir.size() + 1);
        Logging::Manifest::getManifest().addContext(_pipeId, contextDir + "context.json"_cs);
    }

    void end_apply() override {
        cstring outputFile = _outputDir + "/"_cs + _options.programName + ".bfa"_cs;
        std::ofstream ctxt_stream(outputFile, std::ios_base::app);

        Logging::Manifest &manifest = Logging::Manifest::getManifest();
        if (_success) {
            // Always output primitives json file (info used by model for logging actions)
            cstring primitivesFile = _outputDir + "/"_cs + _options.programName + ".prim.json"_cs;
            LOG2("ASM generation for primitives: " << primitivesFile);
            ctxt_stream << "primitives: \"" << _options.programName << ".prim.json\"" << std::endl;
            std::ofstream prim(primitivesFile);
            _primitives.serialize(prim);
            prim << std::endl << std::flush;

            // Output dynamic hash json file
            cstring dynHashFile = _outputDir + "/"_cs + _options.programName + ".dynhash.json"_cs;
            LOG2("ASM generation for dynamic hash: " << dynHashFile);
            ctxt_stream << "dynhash: \"" << _options.programName << ".dynhash.json\"" << std::endl;
            std::ofstream dynhash(dynHashFile);
            dynhash << _dynhash << std::endl << std::flush;
        }
        if (_options.debugInfo) {  // Generate graphs only if invoked with -g
            auto graphsDir = BFNContext::get().getOutputDirectory("graphs"_cs, _pipeId);
            // Output dependency graph json file
            if (_depgraph.size() > 0) {
                cstring depFileName = "dep"_cs;
                cstring depFile = graphsDir + "/"_cs + depFileName + ".json"_cs;
                LOG2("Dependency graph json generation for P4i: " << depFile);
                std::ofstream dep(depFile);
                _depgraph.serialize(dep);
                // relative path to the output directory
                // TBD: In manifest, add an option to indicate a program graph
                // which includes both ingress and egress. Currently the graph node
                // in manifest only accepts one gress since the dot graphs are
                // generated per gress. To satisfy schema we use INGRESS, this does
                // not have any affect on p4i interpretation.
                manifest.addGraph(_pipeId, "table"_cs, depFileName, INGRESS, ".json"_cs);
            }
        }

        // We produce the skeleton of context.json regardless the compilation succeeds or fails.
        // It is needed by visualization tool which can visualize some results even from failed
        // compilation.
        // If the compilation succeeds, then either assembler rewrites the skeleton
        // of context.json with real data or if assembler fails or crashes, skeleton
        // of context.json produced here is preserved and can be used by visualization tools.
        outputContext();
    }

 public:
    explicit GenerateOutputs(const BFN::Backend &b, const BFN_Options &o, int pipeId,
                             const Util::JsonObject &p, const Util::JsonObject &d,
                             bool success = true)
        : _options(o),
          _pipeId(pipeId),
          _success(success),
          _dynhash(b.get_phv()),
          _primitives(p),
          _depgraph(d) {
        setStopOnError(false);
        _outputDir = BFNContext::get().getOutputDirectory(""_cs, pipeId);
        if (_outputDir == "") exit(1);
        auto logsDir = BFNContext::get().getOutputDirectory("logs"_cs, pipeId);
        std::string phvLogFile(logsDir + "/phv.json");
        std::string resourcesLogFile(logsDir + "/resources.json");
        addPasses(
            {&_dynhash,  // Verifies that the hash is valid before the dump of
                         // information in assembly
             new BFN::AsmOutput(_pipeId, b.get_phv(), b.get_clot(), b.get_defuse(),
                                b.get_flexible_logging(), b.get_nxt_tbl(), b.get_power_and_mpr(),
                                b.get_tbl_summary(), b.get_live_range_report(),
                                b.get_parser_hdr_seqs(), o, success),
             o.debugInfo ? new PhvLogging(phvLogFile.c_str(), b.get_phv(), b.get_clot(),
                                          *b.get_phv_logging(), *b.get_phv_logging_defuse_info(),
                                          b.get_table_alloc(), b.get_tbl_summary())
                         : nullptr,
             o.debugInfo
                 ? new BFN::ResourcesLogging(b.get_clot(), resourcesLogFile, o.outputDir.c_str())
                 : nullptr});

        setName("Assembly output");
    }
};

/// use pipe.n to generate output directory.
void execute_backend(const IR::BFN::Pipe *maupipe, BFN_Options &options) {
    if (::errorCount() > 0) return;
    if (!maupipe) return;

    if (Log::verbose()) std::cout << "Compiling " << maupipe->canon_name() << std::endl;

    BFN::Backend backend(options, maupipe->canon_id());
    backend.addDebugHook(EventLogger::getDebugHook(), true);
#if BFP4C_CATCH_EXCEPTIONS
    struct failure_guard : boost::noncopyable {
        failure_guard(BFN::Backend &backend, const IR::BFN::Pipe *maupipe)
            : backend(backend), maupipe(maupipe) {}
        ~failure_guard() {
            if (std::uncaught_exception()) {
                for (int pipe_id : maupipe->ids) {
                    GenerateOutputs as(backend, backend.get_options(), pipe_id,
                                       backend.get_prim_json(), backend.get_json_graph(), false);
                    if (maupipe) maupipe->apply(as);
                }

                if (Log::verbose()) std::cerr << "Failed." << std::endl;
            }
        }
        BFN::Backend &backend;
        const IR::BFN::Pipe *maupipe;
    };
    failure_guard guard(backend, maupipe);
#endif  // BFP4C_CATCH_EXCEPTIONS
    maupipe = maupipe->apply(backend);
    bool mau_success = maupipe != nullptr;
    bool comp_success = (::errorCount() > 0) ? false : true;
    if (maupipe) {
        for (int pipe_id : maupipe->ids) {
            GenerateOutputs as(backend, backend.get_options(), pipe_id, backend.get_prim_json(),
                               backend.get_json_graph(), mau_success && comp_success);
            maupipe->apply(as);
        }
    }
}

// Reports warning and error counts.
// Always call this function shortly before exiting.
static void report_stats() {
    // Han indicated ≤1 ‘_’ per identifier is a required rule.
    auto &myErrorReporter{BFNContext::get().errorReporter()};
    //  The next 2 lines: creating variables just for readability of code.
    const unsigned long long error_count = myErrorReporter.getErrorCount();
    const unsigned long long warning_count = myErrorReporter.getWarningCount();

    using namespace std;

    if (get_has_output_already_been_silenced()) {  //  we need to re-enable cerr
        //  no good ctors for this acc. to <https://m.cplusplus.com/reference/fstream/filebuf/open/>
        //  :-(
        static filebuf devStdErr;

        devStdErr.open("/dev/stderr", ios::out | ios::app);
        cerr.rdbuf(&devStdErr);

        // should we call “reset_has_output_already_been_silenced()” here?
    }

    cerr << endl;
    if (error_count > 0 || warning_count > 0) {
        cerr << error_count << " error" << (error_count == 1 ? "" : "s") << ", " << warning_count
             << " warning" << (warning_count == 1 ? "" : "s") << " generated." << endl;
        cerr << endl;
    }
}  //  end of reporting procedure

// define a set of constants to return so we can decide what to do for
// context.json generation, as we need to generate as much as we can
// for failed programs
constexpr unsigned SUCCESS = 0;
// PerPipeResourceAllocation error. This can be fitting or other issues in
// the backend, where we may have hope to generate partial context and
// visualizations.
constexpr unsigned COMPILER_ERROR = 1;
// Program or programmer errors. Nothing to do until the program is fixed
constexpr unsigned INVOCATION_ERROR = 2;
constexpr unsigned PROGRAM_ERROR = 3;
#if BFP4C_CATCH_EXCEPTIONS
// Internal compiler error
constexpr unsigned INTERNAL_COMPILER_ERROR = 4;
#endif  // BFP4C_CATCH_EXCEPTIONS

int handle_return(const int error_code, [[maybe_unused]] const BFN_Options &options) {
    report_stats();
#if BAREFOOT_INTERNAL
    if (BFNContext::get().errorReporter().verify_checks() ==
        BfErrorReporter::CheckResult::SUCCESS) {
        return SUCCESS;
    }
#endif /* BAREFOOT_INTERNAL */
    return error_code;
}

int main(int ac, char **av) {
    setup_gc_logging();
    setup_signals();
    // initialize the Barefoot specific error types
    BFN::ErrorType::getErrorTypes();

    AutoCompileContext autoBFNContext(new BFNContext);
    auto &options = BackendOptions();

    if (!options.process(ac, av) || ::errorCount() > 0)
        return handle_return(INVOCATION_ERROR, options);

    options.setInputFile();
    Device::init(options.target);

    // Initialize EventLogger
    if (BackendOptions().debugInfo) {
        // At least skeleton of events.json should be emitted alongside with other JSON files
        // so P4I knows what is the setup of the system.
        // Only enable actual events per user demand
        EventLogger::get().init(BFNContext::get().getOutputDirectory().c_str(), "events.json");
        if (BackendOptions().enable_event_logger) EventLogger::get().enable();
    }

#if BFP4C_CATCH_EXCEPTIONS
    try {
#endif  // BFP4C_CATCH_EXCEPTIONS
        auto *program = run_frontend();

        if (options.num_stages_override) {
            Device::overrideNumStages(options.num_stages_override);
            if (::errorCount() > 0) {
                return handle_return(INVOCATION_ERROR, options);
            }
        }

        // If there was an error in the frontend, we are likely to end up
        // with an invalid program for serialization, so we bail out here.
        if (!program || ::errorCount() > 0) return handle_return(PROGRAM_ERROR, options);

        // Initialize Architecture::currentArchitecture() because this function
        // maybe used in the runtime library or the constructor of midend passes
        // it would be too late to run as part of the midend passes
        program->apply(BFN::FindArchitecture());

        if (::errorCount() > 0) return handle_return(PROGRAM_ERROR, options);

        // If we just want to prettyprint to p4_16, running the frontend is sufficient.
        if (!options.prettyPrintFile.empty())
            return handle_return(::errorCount() > 0 ? PROGRAM_ERROR : SUCCESS, options);

        log_dump(program, "Initial program");

        // Dump frontend IR for p4i if debug (-g) was selected
        // Or if the --toJson was used
        if (BackendOptions().debugInfo || !options.dumpJsonFile.empty()) {
            // Dump file is either whatever --toJson specifies or a default one for p4i
            std::filesystem::path irFilePath =
                !options.dumpJsonFile.empty()
                    ? options.dumpJsonFile
                    : std::filesystem::path(BFNContext::get().getOutputDirectory() +
                                            "/frontend-ir.json");
            // Print out the IR for p4i after frontend (--toJson "-" signifies stdout)
            auto &irFile = irFilePath != "-" ? *openFile(irFilePath, false) : std::cout;
            LOG3("IR dump after frontend to " << irFilePath);
            JSONGenerator(irFile, true) << program << std::endl;
        }

#if BAREFOOT_INTERNAL
        BFN::collect_diagnostic_checks(BFNContext::get().errorReporter(), options);
#endif /* BAREFOOT_INTERNAL */
        BFN::generateRuntime(program, options);
        if (::errorCount() > 0) return handle_return(PROGRAM_ERROR, options);

        auto hook = options.getDebugHook();
        BFN::MidEnd midend(options);
        midend.addDebugHook(hook, true);
        midend.addDebugHook(EventLogger::getDebugHook(), true);

        // so far, everything is still under the same program for 32q, generate two separate threads
        program = program->apply(midend);
        if (!program)
            // still did not reach the backend for fitting issues
            return handle_return(PROGRAM_ERROR, options);
        log_dump(program, "After midend");
        if (::errorCount() > 0) return handle_return(PROGRAM_ERROR, options);

        /* save the pre-packing p4 program */
        // return IR::P4Program with @flexible header packed
        auto map = new RepackedHeaderTypes;
        BridgedPacking bridgePacking(options, *map, *midend.sourceInfoLogging);
        bridgePacking.addDebugHook(hook, true);
        bridgePacking.addDebugHook(EventLogger::getDebugHook(), true);

        program->apply(bridgePacking);
        if (!program || ::errorCount() > 0) return handle_return(PROGRAM_ERROR, options);
        // still did not reach the backend for fitting issues

        SubstitutePackedHeaders substitute(options, *map, *midend.sourceInfoLogging);
        substitute.addDebugHook(hook, true);
        substitute.addDebugHook(EventLogger::getDebugHook(), true);

        program = program->apply(substitute);
        log_dump(program, "After flexiblePacking");
        if (!program)
            // still did not reach the backend for fitting issues
            return handle_return(PROGRAM_ERROR, options);

        if (!substitute.getToplevelBlock()) return handle_return(PROGRAM_ERROR, options);

        if (options.debugInfo) {
            program->apply(SourceInfoLogging(BFNContext::get().getOutputDirectory().string(),
                                             "source.json", *midend.sourceInfoLogging));
        }

#if !BAREFOOT_INTERNAL
        // turn all errors into "fatal errors" by exiting on the first error encountered
        BFNContext::get().errorReporter().setMaxErrorCount(1);
#endif

        // create the archive manifest
        Logging::Manifest &manifest = Logging::Manifest::getManifest();

        // Register event logger in manifest
        // Also register frontend IR dump
        if (BackendOptions().debugInfo) {
            manifest.setEventLog("events.json"_cs);
            manifest.setFrontendIrLog("frontend-ir.json"_cs);
        }

        // setup the pipes and the architecture config early, so that the manifest is
        // correct even if there are errors in the backend.
        for (auto [pipe_idx, pipe] : substitute.pipes) {
            manifest.setPipe(pipe_idx, pipe->canon_name());
        }
        manifest.addArchitecture(substitute.getPipelines());

        class manifest_generator_guard : boost::noncopyable {
            Logging::Manifest &manifest;

         public:
            explicit manifest_generator_guard(Logging::Manifest &manifest) : manifest(manifest){};
            ~manifest_generator_guard() {
                try {
                    manifest.setSuccess(::errorCount() == 0);
                } catch (...) {
                    manifest.setSuccess(false);
                }
                manifest.serialize();
            }
        };
        manifest_generator_guard emit_manifest(manifest);

        for (auto &pipe : substitute.pipe) {
#if BAREFOOT_INTERNAL
            if (std::all_of(pipe->names.begin(), pipe->names.end(),
                            [&](cstring n) { return options.skipped_pipes.count(n); })) {
                continue;
            }
#endif
            if (options.create_graphs) {
                // generate graphs
                // In principle this should not fail, so we call it before the backend
                for (size_t pipe_idx = 0; pipe_idx < pipe->ids.size(); ++pipe_idx) {
                    int pipe_id = pipe->ids[pipe_idx];
                    cstring pipe_name = pipe->names[pipe_id];
                    LOG3("Creating graphs for pipe " << pipe_id);
                    EventLogger::get().pipeChange(pipe_id);
                    manifest.setPipe(pipe_id, pipe_name);
                    auto outputDir = BFNContext::get().getOutputDirectory("graphs"_cs, pipe_id);
                    const auto graphsDir = std::filesystem::path(outputDir.string_view());
                    // set the pipe for the visitors to compute the output dir
                    manifest.setRefMap(&substitute.refMap);
                    auto toplevel = substitute.getToplevelBlock();
                    if (toplevel != nullptr) {
                        LOG2("Generating control graphs");
                        // FIXME(cc): this should move to the manifest graph generation to work
                        // per-pipe
                        graphs::ControlGraphs cgen(&substitute.refMap, &substitute.typeMap,
                                                   graphsDir);
                        toplevel->getMain()->apply(cgen);
                        // p4c frontend only saves the parser graphs into controlGraphsArray
                        // (and does not output them)
                        // Therefore we just create empty parser graphs
                        std::vector<graphs::Graphs::Graph *> emptyParser;
                        // And call graph visitor that actually outputs the graphs from the arrays
                        std::filesystem::path filePath = "";
                        graphs::Graph_visitor gvs(graphsDir, true, false, false, filePath);
                        gvs.process(cgen.controlGraphsArray, emptyParser);
                        // generate entries for controls in manifest
                        toplevel->getMain()->apply(manifest);
                    }
                    LOG2("Generating parser graphs");
                    program->apply(manifest);  // generate graph entries for parsers in manifest
                }
            }

            LOG3("Executing backend for pipe : " << pipe->canon_name());
            EventLogger::get().pipeChange(pipe->canon_id());
            execute_backend(pipe, options);
        }

        report_stats();

        if (Log::verbose()) std::cout << "Done." << std::endl;
#if BAREFOOT_INTERNAL
        if (BFNContext::get().errorReporter().verify_checks() ==
            BfErrorReporter::CheckResult::SUCCESS) {
            return SUCCESS;
        }
#endif
        return ::errorCount() > 0 ? COMPILER_ERROR : SUCCESS;
        //     ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
        // _intentionally_ not "return_from_main_politely"
        //   since the stats code has already been called, by this point.

#if BFP4C_CATCH_EXCEPTIONS
        // catch all exceptions here
    } catch (const Util::CompilerBug &e) {
        BFNContext::get().errorReporter().increment_the_error_count();
        report_stats();

#ifdef BAREFOOT_INTERNAL
        bool barefootInternal = true;
#else
        bool barefootInternal = false;
#endif

        if (std::string(e.what()).find(".p4(") != std::string::npos || barefootInternal)
            std::cerr << e.what() << std::endl;
        std::cerr << "Internal compiler error. Please submit a bug report with your code."
                  << std::endl;
        return INTERNAL_COMPILER_ERROR;
    } catch (const Util::CompilerUnimplemented &e) {
        BFNContext::get().errorReporter().increment_the_error_count();
        report_stats();

        std::cerr << e.what() << std::endl;
        return COMPILER_ERROR;
    } catch (const Util::CompilationError &e) {
        std::cerr << e.what() << std::endl;
        return handle_return(PROGRAM_ERROR, options);
#if BAREFOOT_INTERNAL
    } catch (const std::exception &e) {
        BFNContext::get().errorReporter().increment_the_error_count();
        report_stats();

        std::cerr << "Internal compiler error: " << e.what() << std::endl;
        return INTERNAL_COMPILER_ERROR;
#endif
    } catch (...) {
        BFNContext::get().errorReporter().increment_the_error_count();
        report_stats();

        std::cerr << "Internal compiler error. Please submit a bug report with your code."
                  << std::endl;
        return INTERNAL_COMPILER_ERROR;
    }
#endif  // BFP4C_CATCH_EXCEPTIONS
}
