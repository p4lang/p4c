#include "parserDriver.h"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/format.hpp>

#include <cerrno>
#include <cstdio>
#include <iostream>
#include <sstream>

#include "frontends/common/constantFolding.h"
#include "frontends/parsers/p4/p4lexer.hpp"
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

AbstractParserDriver::~AbstractParserDriver() { }

void AbstractParserDriver::onReadToken(const char* text) {
    auto posBeforeToken = Util::InputSources::instance->getCurrentPosition();
    Util::InputSources::instance->appendText(text);
    auto posAfterToken = Util::InputSources::instance->getCurrentPosition();
    yylloc = Util::SourceInfo(posBeforeToken, posAfterToken);
}

void AbstractParserDriver::onReadLineNumber(const char* text) {
    char* last;
    errno = 0;
    lineDirectiveLine = strtol(text, &last, 10);
    if (errno != 0 ||
        // we have not parsed the complete string
        strlen(last) != 0)
        ErrorReporter::instance.parser_error("Error parsing line number %s", text);
}

void AbstractParserDriver::onReadFileName(const char* text) {
    lineDirectiveFile = cstring(text);
    Util::InputSources::instance->mapLine(lineDirectiveFile, lineDirectiveLine);
}

void AbstractParserDriver::onReadIdentifier(cstring id) {
    lastIdentifier = id;
}

void AbstractParserDriver::onParseError(const Util::SourceInfo& location,
                                        const std::string& message) {
    static const std::string unexpectedIdentifierError =
        "syntax error, unexpected IDENTIFIER";
    if (boost::equal(message, unexpectedIdentifierError)) {
        ErrorReporter::instance.parser_error(location, boost::format("%s \"%s\"") %
                                                       unexpectedIdentifierError %
                                                       lastIdentifier);
    } else {
        ErrorReporter::instance.parser_error(location, message);
    }
}

P4ParserDriver::P4ParserDriver()
  : structure(new Util::ProgramStructure)
  , declarations(new IR::IndexedVector<IR::Node>())
{ }

/* static */ const IR::P4Program*
P4ParserDriver::parse(const char* name, std::istream& in) {
    if (Log::verbose())
        std::cout << "Parsing P4-16 program " << name << std::endl;

    // Create and configure the parser and lexer.
    P4ParserDriver driver;
    P4Lexer lexer(in);
    P4Parser parser(driver, lexer);

#ifdef YYDEBUG
    if (const char *p = getenv("YYDEBUG"))
        parser.set_debug_level(atoi(p));
    driver.structure->setDebug(parser.debug_level() != 0);
#endif

    // Parse.
    if (parser.parse() != 0) return nullptr;
    driver.structure->endParse();
    return new IR::P4Program(driver.declarations->srcInfo, *driver.declarations);
}

/* static */ const IR::P4Program*
P4ParserDriver::parse(const char* name, FILE* in) {
    AutoStdioInputStream inputStream(in);
    return parse(name, inputStream.get());
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
        declarations->push_back(error);
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
V1ParserDriver::parse(const char* name, std::istream& in) {
    if (Log::verbose())
        std::cout << "Parsing P4-14 program " << name << std::endl;

    // Create and configure the parser and lexer.
    V1ParserDriver driver;
    V1Lexer lexer(in);
    V1Parser parser(driver, lexer);

#ifdef YYDEBUG
    if (const char *p = getenv("YYDEBUG"))
        parser.set_debug_level(atoi(p));
#endif

    // Reset our location. This is necessary because before any P4-14 code gets
    // parsed, P4V1::Converter::loadModel() has already parsed `v1model.p4`
    // (which is a P4-16 file) and moved our location.
    Util::InputSources::instance->mapLine(name, 1);

    // Parse.
    if (parser.parse() != 0) return nullptr;
    return driver.global;
}

/* static */ const IR::V1Program*
V1ParserDriver::parse(const char* name, FILE* in) {
    AutoStdioInputStream inputStream(in);
    return parse(name, inputStream.get());
}

IR::Constant* V1ParserDriver::constantFold(IR::Expression* expr) {
    IR::Node* node(expr);
    auto rv = node->apply(P4::DoConstantFolding(nullptr, nullptr))->to<IR::Constant>();
    return rv ? new IR::Constant(rv->type, rv->value, rv->base) : nullptr;
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
