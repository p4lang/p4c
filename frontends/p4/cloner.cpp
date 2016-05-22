#include "cloner.h"

namespace P4 {

const IR::Node* DeepCloner::postorder(IR::Declaration_Variable* decl) {
    return new IR::Declaration_Variable(decl->srcInfo, decl->name, decl->annotations,
                                        decl->type, decl->initializer); }

const IR::Node* DeepCloner::postorder(IR::Declaration_Instance* decl) {
    return new IR::Declaration_Instance(decl->srcInfo, decl->name, decl->type,
                                        decl->annotations, decl->initializer); }

const IR::Node* DeepCloner::postorder(IR::ParserState* decl) {
    return new IR::ParserState(decl->srcInfo, decl->name, decl->annotations,
                               decl->components, decl->selectExpression); }

const IR::Node* DeepCloner::postorder(IR::P4Parser* decl) {
    return new IR::P4Parser(decl->srcInfo, decl->name, decl->type, decl->constructorParams,
                            decl->stateful, decl->states); }

const IR::Node* DeepCloner::postorder(IR::P4Control* decl) {
        return new IR::P4Control(decl->srcInfo, decl->name, decl->type,
                                 decl->constructorParams, decl->stateful, decl->body); }

const IR::Node* DeepCloner::postorder(IR::P4Action* decl)  {}

const IR::Node* DeepCloner::postorder(IR::ActionListElement* decl)  {}

const IR::Node* DeepCloner::postorder(IR::TableProperty* decl)  {}

const IR::Node* DeepCloner::postorder(IR::P4Table* decl)  {}

const IR::Node* DeepCloner::postorder(IR::Function* decl)  {}

const IR::Node* DeepCloner::postorder(IR::Parameter* decl)  {}

}  // namespace P4
