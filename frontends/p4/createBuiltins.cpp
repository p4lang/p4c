#include "createBuiltins.h"
#include "ir/ir.h"

namespace P4 {
void CreateBuiltins::postorder(IR::P4Parser* parser) {
    IR::Vector<IR::ParserState> *newStates = new IR::Vector<IR::ParserState>(*parser->states);
    IR::ParserState* ac = new IR::ParserState(Util::SourceInfo(),
                                              IR::ParserState::accept,
                                              IR::Annotations::empty,
                                              new IR::Vector<IR::StatOrDecl>(),
                                              nullptr);
    IR::ParserState* rj = new IR::ParserState(Util::SourceInfo(),
                                              IR::ParserState::reject,
                                              IR::Annotations::empty,
                                              new IR::Vector<IR::StatOrDecl>(),
                                              nullptr);
    newStates->push_back(ac);
    newStates->push_back(rj);
    parser->states = newStates;
}
}  // namespace P4
