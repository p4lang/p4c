/*
Copyright 2013-present Barefoot Networks, Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#ifndef _P4_FRONTEND_H_
#define _P4_FRONTEND_H_

#include <iostream>
#include <fstream>
#include "ir/ir.h"
#include "parseAnnotations.h"
#include "../common/options.h"
#include "lib/path.h"
#include "lib/nullstream.h"
#include "toP4/toP4.h"

namespace P4 {

    /**
This pass outputs the program as a P4 source file.
*/
class PrettyPrint : public Inspector {
    /// output file
    cstring ppfile;
    /// The file that is being compiled.  This used
    cstring inputfile;
 public:
    explicit PrettyPrint(const CompilerOptions& options) {
        setName("PrettyPrint");
        ppfile = options.prettyPrintFile;
        inputfile = options.file;
    }
    bool preorder(const IR::P4Program* program) override {
        if (!ppfile.isNullOrEmpty()) {
            Util::PathName path(ppfile);
            std::ostream *ppStream = openFile(path.toString(), true);
            P4::ToP4 top4(ppStream, false, inputfile);
            (void)program->apply(top4);
        }
        return false;  // prune
    }
};

/**
 * This pass is a no-op whose purpose is to mark the end of the
 * front-end, which is useful for debugging. It is implemented as an
 * empty @ref PassManager (instead of a @ref Visitor) for efficiency.
 */
class FrontEndLast : public PassManager {
 public:
    FrontEndLast() { setName("FrontEndLast"); }
};

/**
 * This pass is a no-op whose purpose is to mark a point in the
 * front-end, used for testing.
 */
class FrontEndDump : public PassManager {
 public:
    FrontEndDump() { setName("FrontEndDump"); }
};

class FrontEnd {
    /// A pass for parsing annotations.
    ParseAnnotations parseAnnotations;

    std::vector<DebugHook> hooks;

 public:
    FrontEnd() = default;
    explicit FrontEnd(ParseAnnotations parseAnnotations)
        : parseAnnotations(parseAnnotations) { }
    explicit FrontEnd(DebugHook hook) { hooks.push_back(hook); }
    explicit FrontEnd(ParseAnnotations parseAnnotations, DebugHook hook)
            : FrontEnd(parseAnnotations) {
        hooks.push_back(hook);
    }
    void addDebugHook(DebugHook hook) { hooks.push_back(hook); }
    // If p4c is run with option '--listFrontendPasses', outStream is used for printing passes names
    const IR::P4Program* run(const CompilerOptions& options, const IR::P4Program* program,
                             bool skipSideEffectOrdering = false,
                             std::ostream* outStream = nullptr);
};

}  // namespace P4

#endif /* _P4_FRONTEND_H_ */
