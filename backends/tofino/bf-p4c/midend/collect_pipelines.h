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
 *  Detects multiple pipelines in a program
 */
#ifndef EXTENSIONS_BF_P4C_MIDEND_COLLECT_PIPELINES_H_
#define EXTENSIONS_BF_P4C_MIDEND_COLLECT_PIPELINES_H_

#include <unordered_map>
#include <set>
#include <vector>
#include "ir/ir.h"
#include "frontends/common/resolveReferences/referenceMap.h"

namespace BFN {

/**
 * \class CollectPipelines
 * \ingroup midend
 * \brief Inspector pass that collects information about all pipelines declared
 * in the Switch. For given pipeline (given by its numeric id), it is possible to
 * extract the pipeline components (e.g. parsers, deparsers and gress control
 * for all gresses and possible ghost gress).
 */
class CollectPipelines : public Inspector {
    // Checks the "main"
    bool preorder(const IR::Declaration_Instance*) override;

 public:
    /// Description ingress or egress
    struct FullGress {
        const IR::BFN::TnaParser *parser = nullptr;
        const IR::BFN::TnaControl *control = nullptr;
        const IR::BFN::TnaDeparser *deparser = nullptr;

        bool operator==(const FullGress &) const;
    };

    /// Description of Tofino pipeline with ingress, egress and possibly ghost.
    /// Comparable so that it can be put into std::map as a key or into std::set.
    class Pipe {
        friend class CollectPipelines;
        void set(unsigned count, unsigned idx, const IR::IDeclaration *ty);

     public:
        FullGress ingress, egress;
        const IR::BFN::TnaControl *ghost = nullptr;
        const IR::Declaration_Instance *dec = nullptr;

        /// Identity comparison. I.e. two pipes are the same if they arise from
        /// the same declaration.
        bool operator==(const Pipe &) const;

        /// A total order given by declaration names (i.e. consistent with ==).
        bool operator<(const Pipe &) const;
    };

    using PipeSet = std::set<Pipe>;

    /// Metadata about all the pipelines in the given P4 program. Can be
    /// iterated over, or one can get a set of pipes corresponding to given
    /// control/parser/deparser.
    class Pipelines {
        friend class CollectPipelines;
        std::vector<Pipe> pipes;
        std::map<const IR::Type_Declaration *, std::unordered_set<int>,
                 ByNameLess<const IR::Type_Declaration>> declarationToPipe;

        PipeSet _get(const IR::Type_Declaration *dec) const {
            auto it = declarationToPipe.find(dec);
            BUG_CHECK(it != declarationToPipe.end(),
                      "Declaration %1% not found in pipes", dec);
            PipeSet out;
            for (int idx : it->second)
                out.insert(pipes[idx]);
            return out;
        }

     public:
        std::vector<Pipe>::const_iterator begin() const { return pipes.begin(); }
        std::vector<Pipe>::const_iterator end() const { return pipes.end(); }

        /// Set of all pipes using this parser.
        PipeSet get(const IR::BFN::TnaParser *p) const { return _get(p); }
        /// Set of all pipes using this deparser.
        PipeSet get(const IR::BFN::TnaDeparser *d) const { return _get(d); }
        /// Set of all pipes using this control.
        PipeSet get(const IR::BFN::TnaControl *c) const { return _get(c); }
    };

 private:
    P4::ReferenceMap *refMap;
    Pipelines *pipelines;
    std::unordered_map<const IR::IDeclaration *, Pipe> pipesByDec;

 public:
    /// Pipelines data filed by the pass. Arguments must be non-null, no other
    /// asumptions taken.
    explicit CollectPipelines(P4::ReferenceMap *refMap, Pipelines *pipelines) :
        refMap(refMap), pipelines(pipelines)
    {}
};

}  // namespace BFN

#endif  // EXTENSIONS_BF_P4C_MIDEND_COLLECT_PIPELINES_H_
