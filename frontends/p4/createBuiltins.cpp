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

#include "createBuiltins.h"
#include "ir/ir.h"

namespace P4 {
void CreateBuiltins::postorder(IR::P4Parser* parser) {
    auto newStates = new IR::IndexedVector<IR::ParserState>(*parser->states);
    IR::ParserState* ac = new IR::ParserState(Util::SourceInfo(),
                                              IR::ParserState::accept,
                                              IR::Annotations::empty,
                                              new IR::IndexedVector<IR::StatOrDecl>(),
                                              nullptr);
    IR::ParserState* rj = new IR::ParserState(Util::SourceInfo(),
                                              IR::ParserState::reject,
                                              IR::Annotations::empty,
                                              new IR::IndexedVector<IR::StatOrDecl>(),
                                              nullptr);
    newStates->push_back(ac);
    newStates->push_back(rj);
    parser->states = newStates;
}
}  // namespace P4
