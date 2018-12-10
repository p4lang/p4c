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
#include "frontends/parsers/p4/abstractP4Lexer.hpp"
#include "frontends/parsers/p4/p4AnnotationLexer.hpp"
#include "ir/ir.h"
#include "lib/cstring.h"
#include "lib/source_file.h"

namespace P4 {

class P4Lexer;
class P4Parser;

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
    static const IR::P4Program* parse(FILE* in, const char* sourceFile,
                                      unsigned sourceLine = 1);

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

    // Singletons ////////////////////////////////////////////////////////////
    static const IR::Expression* parseExpression(
        const Util::SourceInfo& srcInfo,
        const IR::Vector<IR::AnnotationToken>& body);

    static const IR::Constant* parseConstant(
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

 protected:
    friend class P4::P4Lexer;
    friend class P4::P4Parser;

    /// Notify that the parser parsed a P4 `error` declaration.
    void onReadErrorDeclaration(IR::Type_Error* error);

    /**
     * There's a lexical ambiguity in P4 between the right shift operator `>>`
     * and nested template arguments. (e.g. `A<B<C>>`) We resolve this at the
     * parser level; from the lexer's perspective, both cases are just a
     * sequence of R_ANGLE tokens. Whitespace is allowed between the R_ANGLEs of
     * nested template arguments, but in the right shift operator case that
     * isn't permitted, and the parser calls this function to detect when that
     * happens and report and error.
     *
     * @param l  The location of the first R_ANGLE token.
     * @param r  The location of the second R_ANGLE token.
     */
    static void checkShift(const Util::SourceInfo& l, const Util::SourceInfo& r);


    ////////////////////////////////////////////////////////////////////////////
    // Shared state manipulated directly by the lexer and parser.
    ////////////////////////////////////////////////////////////////////////////

    /// Semantic information about the program being parsed.
    Util::ProgramStructure* structure = nullptr;

    /// The top-level nodes that make up the P4 program (or program fragment)
    /// we're parsing.
    IR::Vector<IR::Node>* nodes = nullptr;

    /// A scratch buffer to hold the current string literal. (They're lexed
    /// incrementally, so we need to hold some state between tokens.)
    std::string stringLiteral;

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
