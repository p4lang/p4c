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

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <unordered_map>
#include <unordered_set>

#include "preprocessor.h"
#include "lib/cstring.h"
#include "lib/error.h"
#include "lib/exceptions.h"
#include "lib/path.h"
#include "lib/source_file.h"

namespace P4 {

class Preprocessor final {
 private:
    // Represents a conditional compilation block in the source file (#if ... #endif)
    struct ConditionalBlock {
        // Line where the #if/#elif/#else directive that started this block is located.
        // This is used for error messages.
        int line = 0;
        // Whether this block is active, that is, source in it will be copied to the output.
        // Note that if the parent block is not active then this one can't be active either.
        bool active = false;
        // Whether this is an #else block and thus can only be followed by an #endif.
        bool elseBlock = false;
    };

    // Precedence values for binary operators allowed in #if expressions.
    enum class OperatorPrecedence {
        None = 0,
        Or,
        And
    };

    // Pointer to one of the parseXXXDirective member functions.
    using DirectiveParser = void (Preprocessor::*)(const char*& cursor);

    // File to preprocess.
    FILE* input = nullptr;
    // Input filename. Used for source-relative imports.
    Util::PathName filename;
    // Current location in the input source file. Used for error messages.
    Util::SourceFileLine location;
    // Currently-defined preprocessor variables. This may be modified as the preprocessor runs.
    std::unordered_set<cstring> definitions = predefinedVariables;
    // Original definition set. This is used when importing a new file.
    std::unordered_set<cstring> originalDefinitions;
    // Stack of currently-open conditional blocks.
    std::vector<ConditionalBlock> conditionals;
    // Search paths for file imports.
    std::vector<cstring> importPaths;
    // Files that have already been imported. Used to implement implicit include guards.
    std::unordered_set<cstring>* importedFiles = nullptr;
    // Current output. Preprocessed source will be appended here.
    std::string output;
    // Whether #define/#undef directives are allowed at this point. They are only allowed
    // before the first token of the source file.
    bool definesAllowed = true;

    // Standard predefined variables.
    static const std::unordered_set<cstring> predefinedVariables;
    // Parser functions for each supported directive.
    static const std::unordered_map<cstring, DirectiveParser> directiveParsers;
    // Set of characters we consider whitespace.
    static const char whitespace[];

    Preprocessor(FILE* input, const Util::PathName& filename,
                 const std::unordered_set<cstring>& definitions,
                 const std::vector<cstring>& importPaths,
                 std::unordered_set<cstring>& importedFiles);

    void preprocess();
    void skipWhitespace(const char*& cursor) const;
    bool isWhitespace(char c) const;
    void skipAll(const char*& cursor) const;
    void parseDirective(const char*& cursor);
    void parseDefineDirective(const char*& cursor);
    void parseUndefDirective(const char*& cursor);
    void parseIfDirective(const char*& cursor);
    void parseElifDirective(const char*& cursor);
    void parseElseDirective(const char*& cursor);
    void parseEndifDirective(const char*& cursor);
    void parseImportDirective(const char*& cursor);
    void parseLineDirective(const char*& cursor);
    bool isConditionalActive() const;
    bool evaluateExpression(const char*& cursor, OperatorPrecedence minPrec, bool& result) const;
    bool evaluateUnaryExpression(const char*& cursor, bool& result) const;
    bool importFile(const Util::PathName& targetName);
    cstring parseString(const char*& cursor, char terminator) const;
    cstring parseIdentifier(const char*& cursor) const;

    template <typename... Args>
    void error(const char* fmt, Args&&... args) const;

 public:
    static cstring preprocess(FILE* input, cstring filename,
                              const std::unordered_set<cstring>& definitions,
                              const std::vector<cstring>& importPaths,
                              std::unordered_set<cstring>& importedFiles) {
        Preprocessor pp(input, filename, definitions, importPaths, importedFiles);
        pp.preprocess();
        if (::errorCount() > 0)
            return cstring::empty;
        return pp.output;
    }
};

const std::unordered_set<cstring> Preprocessor::predefinedVariables = {
    "__p4__"
};

const std::unordered_map<cstring, Preprocessor::DirectiveParser> Preprocessor::directiveParsers = {
    { "define", &Preprocessor::parseDefineDirective },
    { "elif",   &Preprocessor::parseElifDirective   },
    { "else",   &Preprocessor::parseElseDirective   },
    { "endif",  &Preprocessor::parseEndifDirective  },
    { "if",     &Preprocessor::parseIfDirective     },
    { "import", &Preprocessor::parseImportDirective },
    { "line",   &Preprocessor::parseLineDirective   },
    { "undef",  &Preprocessor::parseUndefDirective  },
};

const char Preprocessor::whitespace[] = " \t\r\n";

Preprocessor::Preprocessor(FILE* input, const Util::PathName& filename, 
                           const std::unordered_set<cstring>& definitions,
                           const std::vector<cstring>& importPaths,
                           std::unordered_set<cstring>& importedFiles)
    : input(input), filename(filename), location(filename.toString(), 0),
      importPaths(importPaths), importedFiles(&importedFiles) {
    this->definitions.insert(definitions.begin(), definitions.end());
    this->originalDefinitions = this->definitions;
    this->importedFiles->insert(Util::PathName(filename).getAbsolutePath().toString());

    struct stat st = {};
    if (fstat(fileno(input), &st) != -1)
        this->output.reserve(st.st_size);
}

void Preprocessor::preprocess() {
    char* line = nullptr;
    size_t len = 0;
    ssize_t read;
    // Whether we're currently in a multi-line comment.
    bool comment = false;
    output.append(cstring("#line 1 \"") + filename.toString() + "\"\n");

    while ((read = getline(&line, &len, input)) != -1) {
        location.sourceLine++;
        const char* cursor = line;
        skipWhitespace(cursor);
        const char* start = cursor;

        while (cursor[0] != '\0') {
            if (comment) {
                const char* end = strstr(cursor, "*/");
                if (end != nullptr) {
                    cursor = end + 2;
                    comment = false;
                } else {
                    skipAll(cursor);
                }
            } else if (cursor[0] == '#') {
                break;
            } else if (cursor[0] == '"') {
                parseString(cursor, '"');
                definesAllowed = false;
            } else if (cursor[0] == '/' && cursor[1] == '/') {
                skipAll(cursor);
            } else if (cursor[0] == '/' && cursor[1] == '*') {
                cursor += 2;
                comment = true;
            } else if (isWhitespace(cursor[0])) {
                cursor++;
            } else {
                cursor++;
                definesAllowed = false;
            }
        }

        size_t length = cursor - line;
        if (length > 0 && line[length - 1] == '\n')
            length--;
        if (isConditionalActive())
            output.append(line, length);

        if (cursor[0] == '#') {
            if (cursor == start) {
                parseDirective(cursor);
            } else {
                error("Preprocessor directives must appear at the beginning of a line");
            }
        }

        output.append("\n");
    }

    if (!conditionals.empty())
        error("Expected to find #endif for conditional block at line %d", conditionals.back().line);
}

void Preprocessor::skipWhitespace(const char*& cursor) const {
    cursor += strspn(cursor, whitespace);
}

bool Preprocessor::isWhitespace(char c) const {
    return strchr(whitespace, c) != nullptr;
}

void Preprocessor::skipAll(const char*& cursor) const {
    cursor += strlen(cursor);
}

void Preprocessor::parseDirective(const char*& cursor) {
    cursor++;
    skipWhitespace(cursor);

    cstring directive = parseIdentifier(cursor);
    auto it = directiveParsers.find(directive);
    if (it == directiveParsers.end()) {
        if (directive == "include")
            error("Invalid preprocessor directive; did you mean #import?");
        else
            error("Invalid preprocessor directive");
        return;
    }

    DirectiveParser parser = it->second;
    (this->*parser)(cursor);

    skipWhitespace(cursor);
    if (cursor[0] == '/' && cursor[1] == '/') {
        skipAll(cursor);
    } else if (cursor[0] != '\0') {
        error("Unexpected text found after preprocessor directive");
        return;
    }
}

void Preprocessor::parseDefineDirective(const char*& cursor) {
    skipWhitespace(cursor);
    cstring definition = parseIdentifier(cursor);
    if (definition.isNullOrEmpty()) {
        error("Expected variable name after #define");
        skipAll(cursor);
        return;
    }
    if (!definesAllowed) {
        error("#define directives are only allowed before the first token of the program");
        return;
    }
    definitions.emplace(definition);
}

void Preprocessor::parseUndefDirective(const char*& cursor) {
    skipWhitespace(cursor);
    cstring definition = parseIdentifier(cursor);
    if (definition.isNullOrEmpty()) {
        error("Expected variable name after #undef");
        skipAll(cursor);
        return;
    }
    if (predefinedVariables.count(definition) != 0) {
        error("Predefined variables cannot be undefined");
        return;
    }
    if (!definesAllowed) {
        error("#undef directives are only allowed before the first token of the program");
        return;
    }
    definitions.erase(definition);
}

void Preprocessor::parseIfDirective(const char*& cursor) {
    ConditionalBlock conditional;
    conditional.line = location.sourceLine;
    if (!evaluateExpression(cursor, OperatorPrecedence::None, conditional.active)) {
        skipAll(cursor);
        return;
    }
    if (!isConditionalActive()) {
        // The parent conditional is not active, so this one isn't either.
        // Note that we evaluated the expression anyway in order to report syntax errors.
        conditional.active = false;
    }
    conditionals.push_back(std::move(conditional));
}

bool Preprocessor::isConditionalActive() const {
    return conditionals.empty() || conditionals.back().active;
}

bool Preprocessor::evaluateExpression(const char*& cursor, OperatorPrecedence minPrec, bool& result) const {
    if (!evaluateUnaryExpression(cursor, result))
        return false;

    while (cursor[0] != ')' && cursor[0] != '\0') {
        if (cursor[0] == '|' && cursor[1] == '|') {
            if (minPrec >= OperatorPrecedence::Or)
                break;
            cursor += 2;
            bool rhs;
            if (!evaluateExpression(cursor, OperatorPrecedence::Or, rhs))
                return false;
            result = result || rhs;
        } else if (cursor[0] == '&' && cursor[1] == '&') {
            if (minPrec >= OperatorPrecedence::And)
                break;
            cursor += 2;
            bool rhs;
            if (!evaluateExpression(cursor, OperatorPrecedence::And, rhs))
                return false;
            result = result && rhs;
        } else {
            break;
        }
    }

    return true;
}

bool Preprocessor::evaluateUnaryExpression(const char*& cursor, bool& result) const {
    result = false;
    skipWhitespace(cursor);

    if (cursor[0] == '(') {
        cursor++;
        if (!evaluateExpression(cursor, OperatorPrecedence::None, result)) {
            return false;
        }
        skipWhitespace(cursor);
        if (cursor[0] != ')') {
            error("Expected closing parenthesis in expression");
            return false;
        }
        cursor++;
    } else if (cursor[0] == '!') {
        cursor++;
        if (!evaluateExpression(cursor, OperatorPrecedence::None, result))
            return false;
        result = !result;
    } else {
        cstring definition = parseIdentifier(cursor);
        if (definition.isNullOrEmpty()) {
            error("Expected variable name in expression");
            return false;
        }
        result = definitions.count(definition) != 0;
    }

    skipWhitespace(cursor);
    return true;
}

void Preprocessor::parseElifDirective(const char*& cursor) {
    if (conditionals.empty() || conditionals.back().elseBlock) {
        error("#elif directives must follow #if or #elif");
        skipAll(cursor);
        return;
    }

    conditionals.pop_back();
    parseIfDirective(cursor);
}

void Preprocessor::parseElseDirective(const char*&) {
    if (conditionals.empty() || conditionals.back().elseBlock) {
        error("#else directives must follow #if or #elif");
        return;
    }

    bool prevActive = conditionals.back().active;
    conditionals.pop_back();

    ConditionalBlock conditional;
    conditional.line = location.sourceLine;
    conditional.elseBlock = true;
    conditional.active = isConditionalActive() && !prevActive;
    conditionals.push_back(std::move(conditional));
}

void Preprocessor::parseEndifDirective(const char*&) {
    if (conditionals.empty()) {
        error("Missing #if directive for #endif");
        return;
    }

    conditionals.pop_back();
}

void Preprocessor::parseImportDirective(const char*& cursor) {
    skipWhitespace(cursor);
    if (cursor[0] != '<' && cursor[0] != '"') {
        error("Expected <> or \"\" file name after #import");
        skipAll(cursor);
        return;
    }
    bool sourceRelative = cursor[0] == '"';
    Util::PathName targetName = parseString(cursor, sourceRelative ? '"' : '>');
    if (targetName.isNullOrEmpty()) {
        error("Expected valid filename after #import");
        skipAll(cursor);
        return;
    }

    if (targetName.isAbsolute()) {
        if (!importFile(targetName)) {
            error("Imported file %s not found", targetName.toString().c_str());
            return;
        }
    } else {
        std::vector<Util::PathName> paths;
        paths.reserve(importPaths.size() + 1);
        if (sourceRelative)
            paths.push_back(filename.getFolder().toString());
        paths.insert(paths.end(), importPaths.begin(), importPaths.end());
        for (const Util::PathName& path : paths) {
            if (importFile(path.join(targetName.toString())))
                return;
        }
        error("Imported file %s not found", targetName.toString().c_str());
    }
}

bool Preprocessor::importFile(const Util::PathName& targetName) {
    cstring absolutePath = targetName.getAbsolutePath().toString();
    if (importedFiles->count(absolutePath) != 0)
        return true;
    importedFiles->emplace(absolutePath);
    FILE* targetFile = fopen(targetName.toString().c_str(), "r");
    if (targetFile == nullptr)
        return false;

    unsigned errors = ::errorCount();
    cstring source = Preprocessor::preprocess(targetFile, targetName.toString(),
                                              originalDefinitions, importPaths, *importedFiles);
    fclose(targetFile);
    if (::errorCount() > errors)
        return false;

    output.append(cstring("#line 1 \"") + targetName.toString() + "\"\n");
    output.append(source);
    output.append(cstring("\n#line ") + std::to_string(location.sourceLine + 1) + " \"" +
                  filename.toString() + "\"");
    return true;
}

void Preprocessor::parseLineDirective(const char*& cursor) {
    // Pass #line directives verbatim to the lexer.
    output.append("#line");
    output.append(cursor);
    skipAll(cursor);
}

cstring Preprocessor::parseString(const char*& cursor, char terminator) const {
    const char* start = ++cursor;
    const char* end = strchr(start, terminator);
    if (end == nullptr) {
        // Truncated string literal. We'll let the lexer raise an error.
        skipAll(cursor);
        return cstring::empty;
    }
    cursor = end + 1;
    return std::string(start, end - start);
}

cstring Preprocessor::parseIdentifier(const char*& cursor) const {
    const char* start = cursor;
    if (!isalpha(cursor[0]) && cursor[0] != '_')
        return cstring::empty;
    while (isalnum(cursor[0]) || cursor[0] == '_')
        ++cursor;
    return std::string(start, cursor - start);
}

template <typename... Args>
void Preprocessor::error(const char* fmt, Args&&... args) const {
    ErrorReporter::instance.preprocessor_error(location, fmt, std::forward<Args>(args)...);
}

} // namespace P4

cstring preprocessP4File(FILE* input, cstring filename,
                         const std::unordered_set<cstring>& definitions,
                         const std::vector<cstring>& importPaths) {
    std::unordered_set<cstring> importedFiles;
    return P4::Preprocessor::preprocess(input, filename, definitions, importPaths, importedFiles);
}
