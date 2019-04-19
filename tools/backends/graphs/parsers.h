/*
 * Copyright (c) 2017 VMware Inc. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _BACKENDS_GRAPHS_PARSERS_H_
#define _BACKENDS_GRAPHS_PARSERS_H_

#include "ir/ir.h"
#include "lib/cstring.h"
#include "lib/nullstream.h"
#include "lib/path.h"
#include "lib/safe_vector.h"

namespace P4 {
// Forward declaration to avoid includes
class TypeMap;
}  // end namespace P4

namespace graphs {

class ParserGraphs : public Inspector {
    const P4::ReferenceMap* refMap;
    const cstring graphsDir;

 protected:
    struct TransitionEdge {
        const IR::ParserState* sourceState;
        const IR::ParserState* destState;
        cstring label;

        TransitionEdge(const IR::ParserState* source, const IR::ParserState* dest,
                       cstring label): sourceState(source),
                                       destState(dest),
                                       label(label) {}
    };

    std::map<const IR::P4Parser*, safe_vector<const TransitionEdge*>> transitions;
    std::map<const IR::P4Parser*, safe_vector<const IR::ParserState*>> states;

 public:
    ParserGraphs(P4::ReferenceMap *refMap, P4::TypeMap *, const cstring &graphsDir) :
            refMap(refMap), graphsDir(graphsDir) {
        CHECK_NULL(refMap); setName("ParserGraphs");
    }

    void postorder(const IR::P4Parser* parser) override;
    void postorder(const IR::ParserState* state) override;
    void postorder(const IR::PathExpression* expression) override;
    void postorder(const IR::SelectExpression* expression) override;
};

}  // namespace graphs

#endif  /* _BACKENDS_GRAPHS_PARSERS_H_ */
