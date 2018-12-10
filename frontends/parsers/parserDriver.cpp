#include "parserDriver.h"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/format.hpp>

#include <cerrno>
#include <cstdio>
#include <iostream>
#include <sstream>

#include "frontends/common/constantFolding.h"
#include "frontends/parsers/p4/p4lexer.hpp"
#include "frontends/parsers/p4/p4AnnotationLexer.hpp"
#include "frontends/parsers/p4/p4parser.hpp"
#include "frontends/parsers/v1/v1lexer.hpp"
#include "frontends/parsers/v1/v1parser.hpp"
#include "lib/error.h"


#ifdef HAVE_LIBBOOST_IOSTREAMS

#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/stream.hpp>

namespace {

/// A RAII helper class that provides an istream wrapper for a stdio FILE*. This
/// is the efficient implementation for users with boost::iostreams installed.
struct AutoStdioInputStream {
    explicit AutoStdioInputStream(FILE* in)
        : source(fileno(in), boost::iostreams::never_close_handle)
        , buffer(source)
        , stream(&buffer)
    { }

    std::istream& get() { return stream; }

 private:
    AutoStdioInputStream(const AutoStdioInputStream&) = delete;
    AutoStdioInputStream(AutoStdioInputStream&&) = delete;

    boost::iostreams::file_descriptor_source source;
    boost::iostreams::stream_buffer<boost::iostreams::file_descriptor_source> buffer;
    std::istream stream;
};

}  // anonymous namespace

#else

/// A RAII helper class that provides an istream wrapper for a stdio FILE*. This
/// is an inefficient fallback implementation.
struct AutoStdioInputStream {
    explicit AutoStdioInputStream(FILE* in) {
        char buffer[512];
        while (fgets(buffer, sizeof(buffer), in)) stream << buffer;
    }

    std::istream& get() { return stream; }

 private:
    std::stringstream stream;
};

#endif


namespace P4 {

AbstractParserDriver::AbstractParserDriver()
    : sources(new Util::InputSources) { }

AbstractParserDriver::~AbstractParserDriver() { }

void AbstractParserDriver::onReadToken(const char* text) {
    auto posBeforeToken = sources->getCurrentPosition();
    sources->appendText(text);
    auto posAfterToken = sources->getCurrentPosition();
    yylloc = Util::SourceInfo(sources, posBeforeToken, posAfterToken);
}

void AbstractParserDriver::onReadLineNumber(const char* text) {
    char* last;
    errno = 0;
    lineDirectiveLine = strtol(text, &last, 10);
    const bool consumedEntireToken = strlen(last) == 0;
    if (errno != 0 || !consumedEntireToken) {
        auto& context = BaseCompileContext::get();
        context.errorReporter().parser_error(sources,
                                             "Error parsing line number %s",
                                             text);
    }
}

void AbstractParserDriver::onReadComment(const char* text, bool lineComment) {
    sources->addComment(yylloc, lineComment, text);
}

void AbstractParserDriver::onReadFileName(const char* text) {
    lineDirectiveFile = cstring(text);
    sources->mapLine(lineDirectiveFile, lineDirectiveLine);
}

void AbstractParserDriver::onReadIdentifier(cstring id) {
    lastIdentifier = id;
}

void AbstractParserDriver::onParseError(const Util::SourceInfo& location,
                                        const std::string& message) {
    static const std::string unexpectedIdentifierError =
        "syntax error, unexpected IDENTIFIER";
    auto& context = BaseCompileContext::get();
    if (boost::equal(message, unexpectedIdentifierError)) {
        context.errorReporter().parser_error(location, boost::format("%s \"%s\"") %
                                                       unexpectedIdentifierError %
                                                       lastIdentifier);
    } else {
        context.errorReporter().parser_error(location, message);
    }
}

P4ParserDriver::P4ParserDriver()
  : structure(new Util::ProgramStructure)
  , nodes(new IR::Vector<IR::Node>())
{ }

bool
P4ParserDriver::parse(AbstractP4Lexer& lexer, const char* sourceFile,
                      unsigned sourceLine /* = 1 */) {
    // Create and configure the parser.
    P4Parser parser(*this, lexer);

#ifdef YYDEBUG
    if (const char *p = getenv("YYDEBUG"))
        parser.set_debug_level(atoi(p));
    structure->setDebug(parser.debug_level() != 0);
#endif

    // Provide an initial source location.
    sources->mapLine(sourceFile, sourceLine);

    // Parse.
    if (parser.parse() != 0) return false;
    structure->endParse();
    return true;
}

/* static */ const IR::P4Program*
P4ParserDriver::parse(std::istream& in, const char* sourceFile,
                      unsigned sourceLine /* = 1 */) {
    LOG1("Parsing P4-16 program " << sourceFile);

    P4ParserDriver driver;
    P4Lexer lexer(in);
    if (!driver.parse(lexer, sourceFile, sourceLine)) return nullptr;
    return new IR::P4Program(driver.nodes->srcInfo, *driver.nodes);
}

/* static */ const IR::P4Program*
P4ParserDriver::parse(FILE* in, const char* sourceFile,
                      unsigned sourceLine /* = 1 */) {
    AutoStdioInputStream inputStream(in);
    return parse(inputStream.get(), sourceFile, sourceLine);
}

template<typename T> const T*
P4ParserDriver::parse(P4AnnotationLexer::Type type,
                      const Util::SourceInfo& srcInfo,
                      const IR::Vector<IR::AnnotationToken>& body) {
    LOG3("Parsing P4-16 annotation " << srcInfo);

    P4AnnotationLexer lexer(type, srcInfo, body);
    if (!parse(lexer, srcInfo.getSourceFile())) {
        return nullptr;
    }

    return nodes->front()->to<T>();
}

/* static */ const IR::Vector<IR::Expression>*
P4ParserDriver::parseExpressionList(const Util::SourceInfo& srcInfo,
                                    const IR::Vector<IR::AnnotationToken>& body) {
    P4ParserDriver driver;
    return driver.parse<IR::Vector<IR::Expression>>(
            P4AnnotationLexer::EXPRESSION_LIST, srcInfo, body);
}

/* static */ const IR::IndexedVector<IR::NamedExpression>*
P4ParserDriver::parseKvList(const Util::SourceInfo& srcInfo,
                            const IR::Vector<IR::AnnotationToken>& body) {
    P4ParserDriver driver;
    return driver.parse<IR::IndexedVector<IR::NamedExpression>>(
            P4AnnotationLexer::KV_LIST, srcInfo, body);
}

/* static */ const IR::Expression*
P4ParserDriver::parseExpression(const Util::SourceInfo& srcInfo,
                                const IR::Vector<IR::AnnotationToken>& body) {
    P4ParserDriver driver;
    return driver.parse<IR::Expression>(
            P4AnnotationLexer::EXPRESSION, srcInfo, body);
}

/* static */ const IR::Constant*
P4ParserDriver::parseConstant(const Util::SourceInfo& srcInfo,
                              const IR::Vector<IR::AnnotationToken>& body) {
    P4ParserDriver driver;
    return driver.parse<IR::Constant>(
            P4AnnotationLexer::INTEGER, srcInfo, body);
}

/* static */ const IR::StringLiteral*
P4ParserDriver::parseStringLiteral(const Util::SourceInfo& srcInfo,
                                   const IR::Vector<IR::AnnotationToken>& body) {
    P4ParserDriver driver;
    return driver.parse<IR::StringLiteral>(
            P4AnnotationLexer::STRING_LITERAL, srcInfo, body);
}

/* static */ const IR::Vector<IR::Expression>*
P4ParserDriver::parseExpressionPair(const Util::SourceInfo& srcInfo,
                                    const IR::Vector<IR::AnnotationToken>& body) {
    P4ParserDriver driver;
    return driver.parse<IR::Vector<IR::Expression>>(
            P4AnnotationLexer::EXPRESSION_PAIR, srcInfo, body);
}

/* static */ const IR::Vector<IR::Expression>*
P4ParserDriver::parseConstantPair(const Util::SourceInfo& srcInfo,
                                  const IR::Vector<IR::AnnotationToken>& body) {
    P4ParserDriver driver;
    return driver.parse<IR::Vector<IR::Expression>>(
            P4AnnotationLexer::INTEGER_PAIR, srcInfo, body);
}

/* static */ const IR::Vector<IR::Expression>*
P4ParserDriver::parseStringLiteralPair(const Util::SourceInfo& srcInfo,
                                       const IR::Vector<IR::AnnotationToken>& body) {
    P4ParserDriver driver;
    return driver.parse<IR::Vector<IR::Expression>>(
            P4AnnotationLexer::STRING_LITERAL_PAIR, srcInfo, body);
}

/* static */ const IR::Vector<IR::Expression>*
P4ParserDriver::parseExpressionTriple(const Util::SourceInfo& srcInfo,
                                     const IR::Vector<IR::AnnotationToken>& body) {
    P4ParserDriver driver;
    return driver.parse<IR::Vector<IR::Expression>>(
            P4AnnotationLexer::EXPRESSION_TRIPLE, srcInfo, body);
}

/* static */ const IR::Vector<IR::Expression>*
P4ParserDriver::parseConstantTriple(const Util::SourceInfo& srcInfo,
                                    const IR::Vector<IR::AnnotationToken>& body) {
    P4ParserDriver driver;
    return driver.parse<IR::Vector<IR::Expression>>(
            P4AnnotationLexer::INTEGER_TRIPLE, srcInfo, body);
}

/* static */ const IR::Vector<IR::Expression>*
P4ParserDriver::parseStringLiteralTriple(const Util::SourceInfo& srcInfo,
                                         const IR::Vector<IR::AnnotationToken>& body) {
    P4ParserDriver driver;
    return driver.parse<IR::Vector<IR::Expression>>(
            P4AnnotationLexer::STRING_LITERAL_TRIPLE, srcInfo, body);
}

/* static */ void P4ParserDriver::checkShift(const Util::SourceInfo& l,
                                             const Util::SourceInfo& r) {
    if (!l.isValid() || !r.isValid())
        BUG("Source position not available!");
    const Util::SourcePosition& f = l.getStart();
    const Util::SourcePosition& s = r.getStart();
    if (f.getLineNumber() != s.getLineNumber() ||
        f.getColumnNumber() != s.getColumnNumber() - 1)
        ::error("Syntax error at shift operator: %1%", l);
}

void P4ParserDriver::onReadErrorDeclaration(IR::Type_Error* error) {
    if (allErrors == nullptr) {
        nodes->push_back(error);
        allErrors = error;
        return;
    }
    allErrors->members.append(error->members);
}

}  // namespace P4

namespace V1 {

V1ParserDriver::V1ParserDriver()
  : global(new IR::V1Program)
{ }

/* static */ const IR::V1Program*
V1ParserDriver::parse(std::istream& in, const char* sourceFile,
                      unsigned sourceLine /* = 1 */) {
    LOG1("Parsing P4-14 program " << sourceFile);

    // Create and configure the parser and lexer.
    V1ParserDriver driver;
    V1Lexer lexer(in);
    V1Parser parser(driver, lexer);

#ifdef YYDEBUG
    if (const char *p = getenv("YYDEBUG"))
        parser.set_debug_level(atoi(p));
#endif

    // Provide an initial source location.
    driver.sources->mapLine(sourceFile, sourceLine);

    // Parse.
    if (parser.parse() != 0) return nullptr;
    return driver.global;
}

/* static */ const IR::V1Program*
V1ParserDriver::parse(FILE* in, const char* sourceFile,
                      unsigned sourceLine /* = 1 */) {
    AutoStdioInputStream inputStream(in);
    return parse(inputStream.get(), sourceFile, sourceLine);
}

IR::Constant* V1ParserDriver::constantFold(IR::Expression* expr) {
    IR::Node* node(expr);
    auto rv = node->apply(P4::DoConstantFolding(nullptr, nullptr))->to<IR::Constant>();
    return rv ? new IR::Constant(rv->srcInfo, rv->type, rv->value, rv->base) : nullptr;
}

IR::Vector<IR::Expression>
V1ParserDriver::makeExpressionList(const IR::NameList* list) {
    IR::Vector<IR::Expression> rv;
    for (auto &name : list->names)
        rv.push_back(new IR::StringLiteral(name));
    return rv;
}

void V1ParserDriver::clearPragmas() {
    currentPragmas.clear();
}

void V1ParserDriver::addPragma(IR::Annotation* pragma) {
    currentPragmas.push_back(pragma);
}

IR::Vector<IR::Annotation> V1ParserDriver::takePragmasAsVector() {
    IR::Vector<IR::Annotation> pragmas;
    std::swap(pragmas, currentPragmas);
    return pragmas;
}

const IR::Annotations* V1ParserDriver::takePragmasAsAnnotations() {
    if (currentPragmas.empty()) return IR::Annotations::empty;
    auto *rv = new IR::Annotations(currentPragmas);
    currentPragmas.clear();
    return rv;
}

}  // namespace V1
