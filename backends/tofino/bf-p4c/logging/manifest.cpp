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

#include <libgen.h>
#include <cstdlib>
#include <string>
#include <filesystem>

#include "../git_sha_version.h"  // for BF_P4C_GIT_SHA
#include "backends/graphs/controls.h"
#include "backends/graphs/graph_visitor.h"
#include "backends/graphs/parsers.h"
#include "bf-p4c/common/run_id.h"
#include "bf-p4c/version.h"  // for BF_P4C_VERSION
#include "ir/ir.h"
#include "manifest.h"

using Manifest = Logging::Manifest;

void Manifest::postorder(const IR::BFN::TnaParser* const parser) {
    const auto pipeName = m_pipes.at(m_pipeId);
    if (parser && (m_pipes.size() == 1 || parser->pipeName == pipeName) && parser->name) {
        CHECK_NULL(m_refMap);
        auto outputDir = BFNContext::get().getOutputDirectory(cstring("graphs"), m_pipeId);
        const auto graphsDir = std::filesystem::path(outputDir.string_view());

        graphs::ParserGraphs pgen(m_refMap, graphsDir);
        parser->apply(pgen);
        // p4c frontend only saves the parser graphs into parserGraphsArray
        // (and does not output them)
        // Therefore we just create empty control graphs
        std::vector<graphs::Graphs::Graph *> emptyControl;
        // And call graph visitor that actually outputs the graphs from the arrays
        std::filesystem::path filePath;
        graphs::Graph_visitor gvs{graphsDir, true, false, false, filePath};
        gvs.process(emptyControl, pgen.parserGraphsArray);
        addGraph(m_pipeId, "parser"_cs, parser->name, parser->thread);
    }
}

void Manifest::postorder(const IR::BFN::TnaControl* const control) {
    const auto controlName = (control ? control->name.name : ""_cs);
    const auto pipeName = m_pipes.at(m_pipeId);
    if (control && control->pipeName == pipeName && controlName) {
        // FIXME(cc): not yet sure why we can't generate control graphs unless invoked at
        // the top level.
        // CHECK_NULL(m_refMap); CHECK_NULL(m_typeMap);
        // auto graphsDir = BFNContext::get().getOutputDirectory("graphs", m_pipeId);
        // graphs::ControlGraphs cgen(m_refMap, m_typeMap, graphsDir);
        // control->apply(cgen);
        // p4c frontend only saves the parser graphs into controlGraphsArray
        // (and does not output them)
        // Therefore we just create empty parser graphs
        // std::vector<graphs::Graphs::Graph *> emptyParser;
        // And call graph visitor that actually outputs the graphs from the arrays
        // cstring filePath("");
        // graphs::Graph_visitor gvs(graphsDir, true, false, false, filePath);
        // gvs.process(cgen.controlGraphsArray, emptyParser);
        addGraph(m_pipeId, "control"_cs, controlName, control->thread);
    }
}

Manifest::InputFiles::InputFiles(const BFN_Options& options) {
    std::filesystem::path filePath(options.file);
    std::filesystem::path rootPath = filePath.parent_path();

    // Assign to m_rootPath, converting to the appropriate string type if necessary
    m_rootPath = rootPath.string();

    // walk the command line arguments and collect includes
    char* const ppFlags = strndup(options.preprocessor_options.c_str(),
                                        options.preprocessor_options.size());
    char* brkt;
    const char* const sep = " ";
    for (const auto* word = strtok_r(ppFlags, sep, &brkt);
         word;
         word = strtok_r(nullptr, sep, &brkt)) {
        if ('0' == word[0]) {
            if      ('I' == word[1]) m_includePaths.insert(cstring(word+2));
            else if ('D' == word[1])      m_defines.insert(cstring(word+2));
            else if ('U' == word[1])      m_defines.erase( cstring(word+2));
        }
    }
    free(ppFlags);
}

void Manifest::InputFiles::serialize(Writer& writer) const {
    writer.Key("source_files");
    writer.StartObject();
    writer.Key("src_root");
    writer.String(m_rootPath.c_str());
    if (m_sourceInfo) {
        writer.Key("src_map");
        writer.String(m_sourceInfo.c_str());
    }
    writer.Key("includes");
    writer.StartArray();
    for (auto &includePath : m_includePaths) writer.String(includePath.c_str());
    writer.EndArray();
    writer.Key("defines");
    writer.StartArray();
    for (auto &define : m_defines) writer.String(define.c_str());
    writer.EndArray();
    writer.EndObject();
}

/// serialize the entire manifest
void Manifest::serialize() {
    rapidjson::StringBuffer string_buffer;
    Writer writer{string_buffer};

    writer.StartObject();  // start BFNCompilerArchive
    writer.Key("schema_version");
    writer.String(MANIFEST_SCHEMA_VERSION);
    writer.Key("target");
    if (m_options.target) writer.String(m_options.target.c_str());
    else                  writer.String("tofino");
    writer.Key("build_date");
    const time_t now = time(NULL);
    char build_date[1024];
    strftime(build_date, 1024, "%c", localtime(&now));
    writer.String(build_date);
    writer.Key("compiler_version");
    writer.String(BF_P4C_VERSION " (" BF_P4C_GIT_SHA ")");
    writer.Key("compilation_succeeded");
    writer.Bool(m_success);
    writer.Key("compilation_time");
    writer.String("0.0");
    writer.Key("run_id");
    writer.String(RunId::getId().c_str());

    serialize_target_data(writer);

    writer.Key("programs");
    writer.StartArray();   // start programs
    writer.StartObject();  // start CompiledProgram
    writer.Key("program_name");
    cstring program_name = m_options.programName + ".p4"_cs;
    writer.String(program_name.c_str());
    writer.Key("p4_version");
    writer.String((m_options.langVersion == BFN_Options::FrontendVersion::P4_14) ?
                  "p4-14" : "p4-16");

    if (m_eventLogPath.size() > 0) {
        writer.Key("event_log_file");
        writer.String(m_eventLogPath.c_str());
    }
    if (m_frontendIrLogPath.size() > 0) {
        writer.Key("frontend_ir_log_file");
        writer.String(m_frontendIrLogPath.c_str());
    }

    m_programInputs.serialize(writer);
    serializePipes(writer);
    writer.EndObject();  // end CompiledProgram
    writer.EndArray();   // end "programs"
    writer.EndObject();  // end BFNCompilerArchive

    BUG_CHECK(m_manifestStream, "manifest.json has been serialized already!");

    m_manifestStream << string_buffer.GetString();
    m_manifestStream.flush();
    m_manifestStream.close();
}

void Manifest::serialize_target_data(Writer& writer) {
    writer.Key("target_data");
    writer.StartObject();

    writer.Key("architecture");
    if (m_options.arch) {
        writer.String(m_options.arch.c_str());
    } else {
        if (m_options.isv1())
            writer.String("v1model");
        else
            writer.String("PISA");
    }

    if (m_pipelines.size() == 0)
        return;

    writer.Key("architectureConfig");
    writer.StartObject();
    writer.Key("name");
    size_t threads = 0;
    for (unsigned int pipe_idx = 0; pipe_idx < m_pipelines.size(); ++pipe_idx) {
        threads += m_pipelines.getPipeline(pipe_idx).threads.size();
    }
    BUG_CHECK(threads > 0, "No pipes found!");
    const auto numPorts = std::to_string(64/threads*2) + "q";
    writer.String(numPorts.c_str());
    writer.Key("pipes");
    writer.StartArray();
    for (auto &pipe : m_pipes) {
        writer.StartObject();
        writer.Key("pipe");
        writer.Int(pipe.first);
        for (auto gress : { INGRESS, EGRESS }) {
            const auto thread = m_pipelines.getPipeline(pipe.first).threads.find(gress);
            if (thread != m_pipelines.getPipeline(pipe.first).threads.end()) {
                writer.Key(gress == INGRESS ? "ingress" : "egress");
                writer.StartObject();
                writer.Key("pipeName");
                writer.String(thread->second->mau->externalName().c_str());
                writer.Key("nextControl");
                writer.StartArray();
                switch (gress) {
                  case INGRESS:
                    // ingress can go to any other pipe's egress
                    for (auto &other_pipe : m_pipes)
                        sendTo(writer, other_pipe.first, EGRESS);
                    break;
                  case EGRESS:
                    if (pipe.first != 0) {
                        // pipe 0 egress always goes out
                        // any other pipe goes to its ingress
                        sendTo(writer, pipe.first, INGRESS);
                    }
                    break;
                  default: break;
                }
                writer.EndArray();
                writer.EndObject();
            }
        }
        writer.EndObject();
    }
    writer.EndArray();
    writer.EndObject();

    writer.EndObject();  //  "target_data"
}

/// serialize all the output in an array of pipes ("pipes" : [ OutputFiles ])
void Manifest::serializePipes(Writer& writer) {
    writer.Key("pipes");
    writer.StartArray();  // for each pipe
    for (auto &pipeOutput : m_pipeOutputs) {
        // TODO: have to add this check for a ghost thread profile.
        if (m_pipes.count(pipeOutput.first)) {
            writer.StartObject();
            writer.Key("pipe_id");
            writer.Int(pipeOutput.first);
            writer.Key("pipe_name");
            writer.String(m_pipes.at(pipeOutput.first).c_str());

            //  The code for a future feature goes here: _per-pipe_ "compilation_succeeded" Booleans
            //  ------------------------------------------------------------------------------------
            //  writer.Key("compilation_succeeded");
            //  writer.Bool( ??? );

            pipeOutput.second->serialize(writer);
            writer.EndObject();
        }
    }
    writer.EndArray();
}

void Manifest::sendTo(Writer& writer, const int pipe, const gress_t gress) {
    const auto thread = m_pipelines.getPipeline(pipe).threads.find(gress);
    if (thread != m_pipelines.getPipeline(pipe).threads.end()) {
        writer.StartObject();
        writer.Key("pipe");
        writer.Int(pipe);
        writer.Key("pipeName");
        writer.String(thread->second->mau->externalName().c_str());
        writer.EndObject();
    }
}

void Manifest::GraphOutput::serialize(Writer& writer) const {
    writer.StartObject();
    writer.Key("path");
    writer.String(m_path.c_str());
    writer.Key("gress");
    writer.String(toString(m_gress).c_str());
    writer.Key("graph_type");
    writer.String(m_type.c_str());
    writer.Key("graph_format");
    writer.String(m_format.c_str());
    writer.EndObject();
}

void Manifest::OutputFiles::serialize(Writer& writer) const {
    writer.Key("files");
    writer.StartObject();
    // Should be overwritten by assembler or driver on successful compile
    writer.Key("context");
    writer.StartObject();  // Abe`s note: is this a bug?  I copied it verbatim from the legacy code.
    writer.Key("path");
    if (m_context) writer.String(m_context);
    else           writer.String("");
    writer.EndObject();

    // Should be written by assembler or driver on successful compile
    if (m_binary) {
        writer.Key("binary");
        writer.StartObject();
        writer.Key("path");
        writer.String(m_context);
        writer.EndObject();
    }

    writer.Key("resources");
    writer.StartArray();
    for (auto &resource : m_resources) {
        writer.StartObject();
        writer.Key("path");
        writer.String(resource.first.c_str());
        writer.Key("type");
        writer.String(resource.second.c_str());
        writer.EndObject();
    }
    writer.EndArray();

    writer.Key("graphs");
    writer.StartArray();
    for (const auto &graph : m_graphs) graph.serialize(writer);
    writer.EndArray();

    writer.Key("logs");
    writer.StartArray();
    for (const auto &log : m_logs) {
        writer.StartObject();
        writer.Key("path");
        writer.String(log.first.c_str());
        writer.Key("log_type");
        writer.String(log.second.c_str());
        writer.EndObject();
    }
    writer.EndArray();
    writer.EndObject();  // files
}

bool Manifest::PathCmp::operator()(const PathAndType& LHS, const PathAndType& RHS) const {
    const auto  left = std::string(LHS.first) + std::string(LHS.second);
    const auto right = std::string(RHS.first) + std::string(RHS.second);
    return std::less<std::string>()(left, right);
}

Manifest::Manifest() : m_options(BackendOptions()), m_programInputs(BackendOptions()) {  //  ctor
    std::filesystem::path path = m_options.outputDir.string_view();
    path /= "manifest.json";
    m_manifestStream.open(path, std::ofstream::out);
    if (! m_manifestStream)
        std::cerr << "Failed to open manifest file ''" << path.string() << "'' for writing."
                  << std::endl;
}

Manifest::~Manifest() {  //  dtor
    for (auto &pipe_output : m_pipeOutputs) delete pipe_output.second;
    m_manifestStream.flush();
    m_manifestStream.close();
}

Manifest::OutputFiles* Manifest::getPipeOutputs(const unsigned int pipe) {
    const auto iterator = m_pipeOutputs.find(pipe);
    if (iterator != m_pipeOutputs.end()) return iterator->second;
    return m_pipeOutputs.emplace(pipe, new OutputFiles()).first->second;
}

void Manifest::setPipe(const int pipeID_in, const cstring pipe_name) {
#if BAREFOOT_INTERNAL
    if (m_options.skipped_pipes.count(pipe_name)) return;
#endif
    m_pipeId = pipeID_in;
    m_pipes.emplace(m_pipeId, pipe_name);
    // and add implicitly add the pipe outputs so that even if there are no
    // files produced by the compiler, we get the pipe structure.
    getPipeOutputs(pipeID_in);
}

void Manifest::addGraph(const int pipe, const cstring graphType, const cstring graphName,
                        const gress_t gress, const cstring ext) {
    const auto path = BFNContext::get().getOutputDirectory("graphs"_cs, pipe)
        .substr(m_options.outputDir.size()+1) + "/" + graphName + ext;
    getPipeOutputs(pipe) -> m_graphs.insert(GraphOutput(path, gress, graphType, ext));
}

void Manifest::addLog(const int pipe, const cstring logType, const cstring logName) {
    const auto path = BFNContext::get().getOutputDirectory("logs"_cs, pipe)
        .substr(m_options.outputDir.size()+1) + "/" + logName;
    getPipeOutputs(pipe) -> m_logs.insert(PathAndType(path, logType));
}

/// Return the singleton object
Manifest& Manifest::getManifest() {
    static Manifest instance;
    return instance;
}
