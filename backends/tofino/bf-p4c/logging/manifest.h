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

#ifndef _EXTENSIONS_BF_P4C_LOGGING_MANIFEST_H_
#define _EXTENSIONS_BF_P4C_LOGGING_MANIFEST_H_

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <cstdarg>
#include <fstream>
#include <map>
#include <set>
#include <string>
#include <utility>

#include "bf-p4c/arch/arch.h"
#include "bf-p4c/bf-p4c-options.h"
#include "bf-p4c/ir/gress.h"
#include "lib/cstring.h"

namespace Logging {

/// A manifest file is the TOC for the compiler outputs.
/// It is a JSON file, specified by the schema defined in archive_manifest.py
/// We derive it from an inspector, since it needs access to the P4 blocks to
/// figure out what graphs were generated. Ideally, it should be passed to the
/// p4-graphs calls and generate the signature there. But we won't pollute
/// that code with BFN internals.
class Manifest : public Inspector {
    using Writer = rapidjson::PrettyWriter<rapidjson::StringBuffer>;
    using PathAndType = std::pair<cstring, cstring>;

    // to use PathAndType as std::set
    struct PathCmp {
        bool operator()(const PathAndType& lhs, const PathAndType& rhs) const;
    };

 private:
    /// the collection of inputs for the program
    struct InputFiles {
        cstring           m_rootPath;
        cstring           m_sourceInfo;  // path to source.json relative to manifest.json
        std::set<cstring> m_includePaths;
        std::set<cstring> m_defines;

        explicit InputFiles(const BFN_Options&);
        void serialize(Writer&) const;
    };

    /// represents a graph file
    struct GraphOutput {
        cstring m_path;
        gress_t m_gress;
        cstring m_type;
        cstring m_format;
        GraphOutput(const cstring path_in, const gress_t gress_in, const cstring type_in,
            const cstring format_in = ".dot"_cs) :
            m_path(path_in), m_gress(gress_in), m_type(type_in), m_format(format_in) { }

        void serialize(Writer&) const;

        cstring getHash() const {
            return m_path + toString(m_gress) + m_type + m_format;
        }
    };

    struct GraphOutputCmp {
        bool operator()(const GraphOutput& lhs, const GraphOutput& rhs) const {
            return std::less<const char *>()(lhs.getHash().c_str(), rhs.getHash().c_str());
        }
    };

    /// the collection of outputs for each pipe
    struct OutputFiles {
        cstring m_context;  // path to the context file
        cstring m_binary;   // path to the binary file
        // pairs of path and resource type
        std::set<PathAndType, PathCmp>        m_resources;
        // pairs of path and log type
        std::set<PathAndType, PathCmp>        m_logs;
        std::set<GraphOutput, GraphOutputCmp> m_graphs;

        void serialize(Writer&) const;
    };

    const BFN_Options& m_options;
    // pairs of <pipe_id, pipe_name>
    std::map<unsigned int, cstring> m_pipes;
    /// map of pipe-id to OutputFiles
    std::map<unsigned int, OutputFiles*> m_pipeOutputs;
    InputFiles m_programInputs;
    cstring m_eventLogPath, m_frontendIrLogPath;
    /// reference to ProgramPipelines to generate the architecture configuration
    BFN::ProgramPipelines m_pipelines;
    int m_pipeId = -1;  /// the current pipe id (for the visitor methods)
    /// to generate parser and control graphs
    P4::ReferenceMap* m_refMap = nullptr;
    std::ofstream m_manifestStream;
    /// compilation succeeded or failed
    bool m_success = false;

 private:
    Manifest();
    ~Manifest();

    OutputFiles* getPipeOutputs(unsigned int pipe);

 public:
    static Manifest& getManifest();  //  singleton

    /// Visitor methods to generate graphs
    void postorder(const IR::BFN::TnaParser* parser)   override;
    void postorder(const IR::BFN::TnaControl* control) override;
    /// helper methods for the graph generators
    /// one can set any of the maps and invoke the appropriate visitors
    void setRefMap(/* intentionally no "const" here */ P4::ReferenceMap* const refMap_in) {
        m_refMap = refMap_in;
    }

    void setPipe(int pipe_ID, const cstring pipe_name);

    void setSuccess(const bool success) { m_success = success; }

    void addContext(const int pipe, const cstring path) {
        getPipeOutputs(pipe) -> m_context = path;
    }
    void addResources(const int pipe, const cstring path, const cstring type = "resources"_cs) {
        getPipeOutputs(pipe) -> m_resources.insert(PathAndType(path, type));
    }
    void addGraph(int pipe, const cstring graphType, const cstring graphName, const gress_t gress,
                  const cstring extension_including_the_leading_period = ".dot"_cs);
    void addLog(int pipe, const cstring logType, const cstring logName);
    void addArchitecture(const BFN::ProgramPipelines& pipelines_in) {
        m_pipelines = pipelines_in;
    }
    void setSourceInfo(const cstring sourceInfo_in) {
        BUG_CHECK(m_programInputs.m_sourceInfo.size() == 0,
            "Trying to redefine path to source info!");
        m_programInputs.m_sourceInfo = sourceInfo_in;
    }
    void setEventLog(const cstring eventLogPath_in) {
        BUG_CHECK(m_eventLogPath.size() == 0, "Trying to redefine path to source info!");
        m_eventLogPath = eventLogPath_in;
    }
    void setFrontendIrLog(const cstring frontendIrLogPath_in) {
        BUG_CHECK(m_frontendIrLogPath.size() == 0, "Trying to redefine path to frontend IR!");
        m_frontendIrLogPath = frontendIrLogPath_in;
    }

    /// serialize the entire manifest
    virtual void serialize();

 private:
    void serialize_target_data(Writer&);

    /// serialize all the output in an array of pipes ("pipes" : [ OutputFiles ])
    void serializePipes(Writer&);

    void sendTo(Writer&, int pipe, gress_t);
};
}  // end namespace Logging

#endif  /* _EXTENSIONS_BF_P4C_LOGGING_MANIFEST_H_ */
