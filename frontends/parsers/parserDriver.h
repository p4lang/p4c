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

#ifndef _FRONTENDS_PARSERS_PARSERDRIVER_H_
#define _FRONTENDS_PARSERS_PARSERDRIVER_H_

#include <cstdio>
#include <iostream>
#include <string>

#include "frontends/p4/symbol_table.h"
#include "frontends/p4/coreLibrary.h"
#include "frontends/parsers/p4/abstractP4Lexer.hpp"
#include "frontends/parsers/p4/p4AnnotationLexer.hpp"
#include "ir/ir.h"
#include "lib/cstring.h"
#include "lib/source_file.h"

namespace P4 {

class P4Lexer;
class P4Parser;

class PkgInfo {
 public:
    PkgInfo() {}
    IR::Node* parent;
    IR::ID id;
    const IR::Type* type;
};

/// The base class of ParserDrivers, which provide a high level interface to
/// parsers and lexers and manage their state.
class AbstractParserDriver {
 public:
    virtual ~AbstractParserDriver() = 0;

 protected:
    AbstractParserDriver();

    ////////////////////////////////////////////////////////////////////////////
    // Callbacks.
    ////////////////////////////////////////////////////////////////////////////

    /**
     * Notify that the lexer has read a comment.
     * @param text         The body of the comment, without the comment termination
     * @param lineComment  If true this is a line comment starting with //
     */
    void onReadComment(const char* text, bool lineComment);

    /// Notify that the lexer read a token. @text is the matched source text.
    void onReadToken(const char* text);

    /// Notify that the lexer read a line number from a #line directive.
    void onReadLineNumber(const char* text);

    /// Notify that the lexer read a filename from a #line directive.
    void onReadFileName(const char* text);

    /// Notify that the lexer read an identifier, @id.
    void onReadIdentifier(cstring id);

    /// Notify that an error was encountered during parsing at @location.
    /// @message is a Bison-provided human-readable explanation of the error.
    void onParseError(const Util::SourceInfo& location,
                      const std::string& message);


    ////////////////////////////////////////////////////////////////////////////
    // Shared state manipulated directly by the lexer and parser.
    ////////////////////////////////////////////////////////////////////////////

    /// The input sources that comprise the P4 program we're parsing.
    Util::InputSources* sources;

    /// The location of the most recent token.
    Util::SourceInfo yylloc;

    /// Scratch storage for the lexer to remember its previous state.
    int saveState = -1;

 private:
    /// The line number from the most recent #line directive.
    int lineDirectiveLine = 0;

    /// The file name from the most recent #line directive.
    cstring lineDirectiveFile;

    /// The last identifier we encountered. This is used for error reporting.
    cstring lastIdentifier;
};

/// A ParserDriver that can parse P4-16 programs.
class P4ParserDriver final : public AbstractParserDriver {
 public:
    /**
     * Parse a P4-16 program.
     *
     * @param in    The input source to read the program from.
     * @param sourceFile  The logical source filename. This doesn't have to be a
     *                    real filename, though it normally will be. This is
     *                    used for logging and to set the initial source
     *                    location.
     * @param sourceLine  The logical source line number. For programs parsed
     *                    from a file, this will normally be 1. This is used to
     *                    set the initial source location.
     * @returns a P4Program object if parsing was successful, or null otherwise.
     */
    static const IR::P4Program* parse(std::istream& in, const char* sourceFile,
                                      unsigned sourceLine = 1);
    static const IR::P4Program* parse(unsigned ver, FILE* in, const char* sourceFile,
                                      cstring outputFile, unsigned sourceLine = 1);

    /**
     * Parses a P4-16 annotation body.
     *
     * @param body  The unparsed annotation body.
     * @returns an AST node if parsing was successful, or null otherwise.
     */
    // Lists /////////////////////////////////////////////////////////////////
    static const IR::Vector<IR::Expression>* parseExpressionList(
        const Util::SourceInfo& srcInfo,
        const IR::Vector<IR::AnnotationToken>& body);

    static const IR::IndexedVector<IR::NamedExpression>* parseKvList(
        const Util::SourceInfo& srcInfo,
        const IR::Vector<IR::AnnotationToken>& body);

    static const IR::Vector<IR::Expression>* parseConstantList(
        const Util::SourceInfo& srcInfo,
        const IR::Vector<IR::AnnotationToken>& body);

    static const IR::Vector<IR::Expression>* parseConstantOrStringLiteralList(
        const Util::SourceInfo& srcInfo,
        const IR::Vector<IR::AnnotationToken>& body);

    static const IR::Vector<IR::Expression>* parseStringLiteralList(
        const Util::SourceInfo& srcInfo,
        const IR::Vector<IR::AnnotationToken>& body);

    // Singletons ////////////////////////////////////////////////////////////
    static const IR::Expression* parseExpression(
        const Util::SourceInfo& srcInfo,
        const IR::Vector<IR::AnnotationToken>& body);

    static const IR::Constant* parseConstant(
        const Util::SourceInfo& srcInfo,
        const IR::Vector<IR::AnnotationToken>& body);

    static const IR::Expression* parseConstantOrStringLiteral(
        const Util::SourceInfo& srcInfo,
        const IR::Vector<IR::AnnotationToken>& body);

    static const IR::StringLiteral* parseStringLiteral(
        const Util::SourceInfo& srcInfo,
        const IR::Vector<IR::AnnotationToken>& body);

    // Pairs /////////////////////////////////////////////////////////////////
    static const IR::Vector<IR::Expression>* parseExpressionPair(
        const Util::SourceInfo& srcInfo,
        const IR::Vector<IR::AnnotationToken>& body);

    static const IR::Vector<IR::Expression>* parseConstantPair(
        const Util::SourceInfo& srcInfo,
        const IR::Vector<IR::AnnotationToken>& body);

    static const IR::Vector<IR::Expression>* parseStringLiteralPair(
        const Util::SourceInfo& srcInfo,
        const IR::Vector<IR::AnnotationToken>& body);

    // Triples ///////////////////////////////////////////////////////////////
    static const IR::Vector<IR::Expression>* parseExpressionTriple(
        const Util::SourceInfo& srcInfo,
        const IR::Vector<IR::AnnotationToken>& body);

    static const IR::Vector<IR::Expression>* parseConstantTriple(
        const Util::SourceInfo& srcInfo,
        const IR::Vector<IR::AnnotationToken>& body);

    static const IR::Vector<IR::Expression>* parseStringLiteralTriple(
        const Util::SourceInfo& srcInfo,
        const IR::Vector<IR::AnnotationToken>& body);

    // P4Runtime Annotations /////////////////////////////////////////////////
    static const IR::Vector<IR::Expression>* parseP4rtTranslationAnnotation(
        const Util::SourceInfo& srcInfo,
        const IR::Vector<IR::AnnotationToken>& body);
    static IR::P4Control* getControl(IR::Node* p) {
        if (p == nullptr) return nullptr;
        return (const_cast<IR::P4Control*>(p->to<IR::P4Control>()));
    }

    static IR::Type_Struct* getStruct(IR::Node* p) {
        if (p == nullptr) return nullptr;
        return (const_cast<IR::Type_Struct*>(p->to<IR::Type_Struct>()));
    }

    static IR::Type_HeaderUnion* getHdrUnion(IR::Node* p) {
        if (p == nullptr) return nullptr;
        return (const_cast<IR::Type_HeaderUnion*>(p->to<IR::Type_HeaderUnion>()));
    }

    static IR::Type_Enum* getEnum(IR::Node* p) {
        if (p == nullptr) return nullptr;
        return (const_cast<IR::Type_Enum*>(p->to<IR::Type_Enum>()));
    }

    static IR::Type_SerEnum* getSerEnum(IR::Node* p) {
        if (p == nullptr) return nullptr;
        return (const_cast<IR::Type_SerEnum*>(p->to<IR::Type_SerEnum>()));
    }

    static IR::Declaration_Instance* getPackage(IR::Node* p) {
        if (p == nullptr) return nullptr;
        return (const_cast<IR::Declaration_Instance*>(p->to<IR::Declaration_Instance>()));
    }

    static void printParser(IR::Node* p) {
        if (p == nullptr) return;
        // auto parser = p->to<IR::P4Parser>();
        //  std::cout << "Printing P4Parser: " << parser << "\n\n";
    }

    static IR::PathExpression* genPathExpr(IR::ID*& id) {
        cstring name = id->name + "_transition";
        return new IR::PathExpression(name);
    }

    //
    // Need to scan the vector to find the node with the name that we're extending,
    // and put a new type in its place
    // Use in yacc code: driver.getNodeByName(driver.nodes, name);
    //
    static IR::Node* getNodeByName(IR::Vector<IR::Node>* nodes, cstring name) {
        cstring node_name;
        // std::cout << "getNodeByName: " << name << "\n";
        for (auto n = nodes->begin(); n != nodes->end(); ++n) {
            if ((*n)->is<IR::P4Parser>()) {
                auto type = (*n)->to<IR::P4Parser>();
                node_name = type->name;
            } else if ((*n)->is<IR::Type_Struct>()) {
                auto type = (*n)->to<IR::Type_Struct>();
                node_name = type->name;
            } else if ((*n)->is<IR::P4Control>()) {
                auto type = (*n)->to<IR::P4Control>();
                node_name = type->name;
            } else if ((*n)->is<IR::Declaration_Instance>()) {
                auto type = (*n)->to<IR::Declaration_Instance>();
                node_name = type->name;
            } else if ((*n)->is<IR::Type_Enum>()) {
                auto type = (*n)->to<IR::Type_Enum>();
                node_name = type->name;
            } else if ((*n)->is<IR::Type_SerEnum>()) {
                auto type = (*n)->to<IR::Type_SerEnum>();
                node_name = type->name;
            } else if ((*n)->is<IR::Type_HeaderUnion>()) {
                auto type = (*n)->to<IR::Type_HeaderUnion>();
                node_name = type->name;
            }
            if (node_name == name)
                return (const_cast<IR::Node*>((*n)));
        }
        return nullptr;
    }

    static int setNodeByName(IR::Vector<IR::Node>* nodes, IR::Node* new_node, cstring name) {
        if ((nodes == nullptr) || (new_node == nullptr)) return -1;
        IR::Node* cur = getNodeByName(nodes, name);
        if (cur == nullptr) return -1;
        // std::cout << "Replacing: " << cur << "\nwith\n" << new_node << "\n";
        std::replace(nodes->begin(), nodes->end(), cur, new_node);
        return 0;
    }

    static const IR::Expression* addDefSelectCase(const IR::Expression** se,
                                                  const IR::ID *id) {
      if ((*se == nullptr)) return nullptr;
      if (id == nullptr) return nullptr;
      auto sep = *se;
      if (!sep->is<IR::SelectExpression>()) {
          std::cout << "Error SelectExpression\n";
          return nullptr;
      }
      auto sepp = sep->to<IR::SelectExpression>();
      auto scs = sepp->selectCases;

      auto keyset = new IR::DefaultExpression();
      auto state = new IR::PathExpression(*id);
      auto sc = new IR::SelectCase(id->srcInfo, keyset, state);
      scs.push_back(sc);

      sepp = new IR::SelectExpression(sepp->srcInfo, sepp->select, scs);
      sep = sepp->to<IR::Expression>();
      return sep;
    }

    static const IR::Expression* delSelectCase(const IR::Expression** se) {
      if ((*se == nullptr)) return nullptr;
      auto sep = *se;
      if (!sep->is<IR::SelectExpression>()) {
          std::cout << "Error SelectExpression\n";
          return nullptr;
      }
      auto sepp = sep->to<IR::SelectExpression>();
      auto scs = sepp->selectCases;
      const IR::SelectCase* found;
      for (auto sc : scs) {
          if (sc->keyset->is<IR::DefaultExpression>()) {
              found = sc;
              break;
          }
      }

      auto it = std::find(scs.begin(), scs.end(), found);
      if (it != scs.end()) {
          scs.erase(it);
          sepp = new IR::SelectExpression(sepp->srcInfo, sepp->select, scs);
          sep = sepp->to<IR::Expression>();
          return sep;
      }
      return nullptr;
    }

    static void delExtractComp(IR::IndexedVector<IR::StatOrDecl>* comp) {
        if (comp == nullptr) return;
        P4CoreLibrary &corelib = P4CoreLibrary::instance;
        const IR::MethodCallStatement* found;
        for (auto c : *comp) {
            if (c->is<IR::MethodCallStatement>()) {
                auto mcs = c->to<IR::MethodCallStatement>();
                auto mc = mcs->methodCall;
                auto member = mc->method->to<IR::Member>();
                if (member->member.name == corelib.packetIn.extract.name) {
                    found = mcs;
                    break;
                }
            }
        }
        auto it = std::find(comp->begin(), comp->end(), found);
        if (it != comp->end()) {
            comp->erase(it);
            return;
        }
        return;
    }

    static const IR::ParserState* findState(IR::Node* p, cstring name) {
        // Both customer and vendor parsers are expected to include parser state
        // start.  Or if they are both included by accident, the check below
        // deals with it.
        if ((p == nullptr) || (name == "start")) return nullptr;
        auto parser = p->to<IR::P4Parser>();
        if (parser == nullptr) return nullptr;
        // std::cout << "P4Parser: " << parser << "\n";
        for (auto s : parser->states) {
            if (s->name == name) {
                return s;
            }
        }
        return nullptr;
    }

    static int addPState(IR::Node** p, const IR::ParserState* ps,
                         IR::Vector<IR::Node>* nodes) {
        if (*p == nullptr || ps == nullptr) return -1;
        auto parser = (*p)->to<IR::P4Parser>();
        if (parser == nullptr) return -1;
        auto sts = parser->states;
        sts.push_back(ps);
        // std::cout << "\nAdd PS: " << ps->name << "\n";
        *p = new IR::P4Parser(parser->srcInfo, parser->name, parser->type,
                              parser->constructorParams, parser->parserLocals, sts);
        if (!setNodeByName(nodes, *p, parser->name))
            return 0;
        else
            return -1;

        return 0;
    }

    static int delPState(IR::Node** p, cstring name, IR::Vector<IR::Node>* nodes) {
        if (*p == nullptr) return -1;
        auto parser = (*p)->to<IR::P4Parser>();
        if (parser == nullptr) return -1;
        auto sts = parser->states;
        if (sts.removeByName(name)) {
            // std::cout << "\nDel PS: " << name << "\n";
            *p = new IR::P4Parser(parser->srcInfo, parser->name, parser->type,
                                  parser->constructorParams, parser->parserLocals, sts);
            if (!setNodeByName(nodes, *p, parser->name))
                return 0;
            else
                return -1;
        }
        return -1;
    }

    static int delObj(IR::Vector<IR::Node>* nodes, cstring name) {
        if (nodes == nullptr) return -1;
        IR::Node* cur = getNodeByName(nodes, name);
        if (cur == nullptr) return -1;
        auto it = std::find(nodes->begin(), nodes->end(), cur);
        if (it != nodes->end()) {
            nodes->erase(it);
            return 0;
        }
        return -1;
    }

 protected:
    friend class P4::P4Lexer;
    friend class P4::P4Parser;

    /// Notify that the parser parsed a P4 `error` declaration.
    void onReadErrorDeclaration(IR::Type_Error* error);

    ////////////////////////////////////////////////////////////////////////////
    // Shared state manipulated directly by the lexer and parser.
    ////////////////////////////////////////////////////////////////////////////

    /// Semantic information about the program being parsed.
    Util::ProgramStructure* structure = nullptr;

    /// The top-level nodes that make up the P4 program (or program fragment)
    /// we're parsing.
    IR::Vector<IR::Node>* nodes = nullptr;

    /// parent (or vendor) P4Parser to track because it is what we override
    /// functionality for and/or add to it.
    IR::Node* parent = nullptr;

    /// parent (or vendor) P4Control to track because it is what we override
    /// functionality for and/or add to it.
    IR::Node* parentCtrl = nullptr;

    IR::Node* parentSt = nullptr;
    IR::Node* parentHu = nullptr;
    IR::Node* parentEm = nullptr;
    IR::Node* parentSerEm = nullptr;
    IR::Node* parentPkg = nullptr;

    const IR::ParserState* overriden_ps = nullptr;

    const IR::ID *defID = nullptr;

    bool custOverrides = false;

    std::map<cstring, PkgInfo*> pkgmap;

    /// A scratch buffer to hold the current string literal. (They're lexed
    /// incrementally, so we need to hold some state between tokens.)
    std::string stringLiteral;

    // flag to track when template args are expected, to adjust the precedence
    // of '<'
    bool template_args = false;

 private:
    P4ParserDriver();

    /// Common functionality for parsing.
    bool parse(AbstractP4Lexer& lexer, const char* sourceFile,
               unsigned sourceLine = 1);

    /// Common functionality for parsing annotation bodies.
    template<typename T> const T* parse(P4AnnotationLexer::Type type,
                                        const Util::SourceInfo& srcInfo,
                                        const IR::Vector<IR::AnnotationToken>& body);

    /// All P4 `error` declarations are merged together in the node, which is
    /// lazily created the first time we see an `error` declaration. (This node
    /// is present in @declarations as well.)
    IR::Type_Error* allErrors = nullptr;
};

}  // namespace P4

namespace V1 {

class V1Lexer;
class V1Parser;

/// A ParserDriver that can parse P4-14 programs.
class V1ParserDriver final : public P4::AbstractParserDriver {
 public:
    /**
     * Parse a P4-14 program.
     *
     * @param in    The input source to read the program from.
     * @param sourceFile  The logical source filename. This doesn't have to be a
     *                    real filename, though it normally will be. This is
     *                    used for logging and to set the initial source
     *                    location.
     * @param sourceLine  The logical source line number. For programs parsed
     *                    from a file, this will normally be 1. This is used to
     *                    set the initial source location.
     * @returns a V1Program object if parsing was successful, or null otherwise.
     */
    static const IR::V1Program* parse(std::istream& in, const char* sourceFile,
                                      unsigned sourceLine = 1);
    static const IR::V1Program* parse(FILE* in, const char* sourceFile,
                                      unsigned sourceLine = 1);

 protected:
    friend class V1::V1Lexer;
    friend class V1::V1Parser;

    /**
     * The P4-14 parser performs constant folding to ensure that constant
     * expressions in the grammar actually produce IR::Constant values in the
     * resulting IR.
     *
     * @return an IR::Constant containing the value of @expr, if @expr is a
     * constant expression, or null otherwise.
     */
    IR::Constant* constantFold(IR::Expression* expr);

    /// @return a vector of IR::Expressions containing an IR::StringLiteral for
    /// name in @list.
    IR::Vector<IR::Expression> makeExpressionList(const IR::NameList* list);

    /// Clear the list of active pragmas.
    void clearPragmas();

    /// Add @pragma to the list of active pragmas.
    void addPragma(IR::Annotation* pragma);

    /// @return a IR::Vector containing the active pragmas, and clear the list.
    IR::Vector<IR::Annotation> takePragmasAsVector();

    /// @return an IR::Annotations object containing the active pragmas, and
    /// clear the list.
    const IR::Annotations* takePragmasAsAnnotations();


    ////////////////////////////////////////////////////////////////////////////
    // Shared state manipulated directly by the lexer and parser.
    ////////////////////////////////////////////////////////////////////////////

    /// The root of the IR tree we're constructing.
    IR::V1Program* global = nullptr;

 private:
    /// The currently active pragmas.
    IR::Vector<IR::Annotation> currentPragmas;

    V1ParserDriver();
};

}  // namespace V1

#endif /* _FRONTENDS_PARSERS_PARSERDRIVER_H_ */
