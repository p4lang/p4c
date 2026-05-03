// SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
// Copyright 2013-present Barefoot Networks, Inc.
//
// SPDX-License-Identifier: Apache-2.0

#include "backends/graphs/version.h"
#include "controls.h"
#include "frontends/common/applyOptionsPragmas.h"
#include "frontends/common/parseInput.h"
#include "frontends/p4/evaluator/evaluator.h"
#include "frontends/p4/frontend.h"
#include "fstream"
#include "graph_visitor.h"
#include "graphs.h"
#include "ir/ir.h"
#include "ir/json_loader.h"
#include "lib/crash.h"
#include "lib/error.h"
#include "lib/exceptions.h"
#include "lib/gc.h"
#include "lib/log.h"
#include "lib/nullstream.h"
#include "parsers.h"

namespace P4::graphs {

class MidEnd : public PassManager {
 public:
    P4::ReferenceMap refMap;
    P4::TypeMap typeMap;
    IR::ToplevelBlock *toplevel = nullptr;

    explicit MidEnd(CompilerOptions &options);
    IR::ToplevelBlock *process(const IR::P4Program *&program) {
        program = program->apply(*this);
        return toplevel;
    }
};

MidEnd::MidEnd(CompilerOptions &options) {
    bool isv1 = options.langVersion == CompilerOptions::FrontendVersion::P4_14;
    refMap.setIsV1(isv1);
    auto evaluator = new P4::EvaluatorPass(&refMap, &typeMap);
    setName("MidEnd");

    addPasses({
        evaluator,
        [this, evaluator]() { toplevel = evaluator->getToplevelBlock(); },
    });
}

class Options : public CompilerOptions {
 public:
    std::filesystem::path graphsDir{"."};
    bool loadIRFromJson = false;  // read from json
    bool graphs = true;           // default behavior
    bool fullGraph = false;
    bool jsonOut = false;
    Options() {
        registerOption(
            "--graphs-dir", "dir",
            [this](const char *arg) {
                graphsDir = arg;
                return true;
            },
            "Use this directory to dump graphs in dot format "
            "(default is current working directory)\n");
        registerOption(
            "--fromJSON", "file",
            [this](const char *arg) {
                loadIRFromJson = true;
                file = arg;
                return true;
            },
            "Use IR representation from JsonFile dumped previously, "
            "the compilation starts with reduced midEnd.");
        registerOption(
            "--graphs", nullptr,
            [this](const char *) {
                graphs = true;
                isGraphsSet = true;
                return true;
            },
            "Use if you want default behavior - generation of separate graphs "
            "for each program block (enabled by default, "
            "if options --fullGraph or --jsonOut are not present).");
        registerOption(
            "--fullGraph", nullptr,
            [this](const char *) {
                fullGraph = true;
                if (!isGraphsSet) graphs = false;
                return true;
            },
            "Use if you want to generate graph depicting control flow "
            "through all program blocks (fullGraph).");
        registerOption(
            "--jsonOut", nullptr,
            [this](const char *) {
                jsonOut = true;
                if (!isGraphsSet) graphs = false;
                return true;
            },
            "Use to generate json output of fullGraph.");
    }

 private:
    bool isGraphsSet = false;
};

using GraphsContext = P4CContextWithOptions<Options>;

}  // namespace P4::graphs

using namespace P4;

int main(int argc, char *const argv[]) {
    setup_gc_logging();
    setup_signals();

    AutoCompileContext autoGraphsContext(new ::graphs::GraphsContext);
    auto &options = ::graphs::GraphsContext::get().options();
    options.langVersion = CompilerOptions::FrontendVersion::P4_16;
    options.compilerVersion = cstring(P4C_GRAPHS_VERSION_STRING);

    if (options.process(argc, argv) != nullptr) {
        if (options.loadIRFromJson == false) options.setInputFile();
    }
    if (::P4::errorCount() > 0) return 1;

    auto hook = options.getDebugHook();

    const IR::P4Program *program = nullptr;

    if (options.loadIRFromJson) {
        std::filebuf fb;
        if (fb.open(options.file, std::ios::in) == nullptr) {
            ::P4::error(ErrorType::ERR_IO, "%s: No such file or directory.", options.file);
            return 1;
        }

        std::istream inJson(&fb);
        JSONLoader jsonFileLoader(inJson);
        if (!jsonFileLoader) {
            ::P4::error(ErrorType::ERR_IO, "Not valid input file");
            return 1;
        }
        program = new IR::P4Program(jsonFileLoader);
        fb.close();
    } else {
        program = P4::parseP4File(options);
        if (program == nullptr || ::P4::errorCount() > 0) return 1;

        try {
            P4::P4COptionPragmaParser optionsPragmaParser(false);
            program->apply(P4::ApplyOptionsPragmas(optionsPragmaParser));

            P4::FrontEnd fe;
            fe.addDebugHook(hook);
            program = fe.run(options, program);
        } catch (const std::exception &bug) {
            std::cerr << bug.what() << std::endl;
            return 1;
        }
        if (program == nullptr || ::P4::errorCount() > 0) return 1;
    }

    graphs::MidEnd midEnd(options);
    midEnd.addDebugHook(hook);
    const IR::ToplevelBlock *top = nullptr;
    try {
        top = midEnd.process(program);
        if (!options.dumpJsonFile.empty()) {
            auto dumpJsonStream = openFile(options.dumpJsonFile, true);
            JSONGenerator(*dumpJsonStream).emit(program);
        }
    } catch (const std::exception &bug) {
        std::cerr << bug.what() << std::endl;
        return 1;
    }
    if (::P4::errorCount() > 0) return 1;

    LOG2("Generating graphs under " << options.graphsDir);
    LOG2("Generating control graphs");
    graphs::ControlGraphs cgen(&midEnd.refMap, &midEnd.typeMap, options.graphsDir);
    top->getMain()->apply(cgen);
    LOG2("Generating parser graphs");
    graphs::ParserGraphs pgg(&midEnd.refMap, options.graphsDir);
    program->apply(pgg);

    graphs::Graph_visitor gvs(options.graphsDir, options.graphs, options.fullGraph, options.jsonOut,
                              options.file);

    gvs.process(cgen.controlGraphsArray, pgg.parserGraphsArray);

    return ::P4::errorCount() > 0;
}
