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
