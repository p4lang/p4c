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

#include "source_file.h"

#include <algorithm>
#include <sstream>

#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_replace.h"
#include "lib/exceptions.h"
#include "lib/log.h"
#include "lib/stringify.h"

void IHasDbPrint::print() const {
    dbprint(std::cout);
    std::cout << std::endl;
}

namespace Util {
SourcePosition::SourcePosition(unsigned lineNumber, unsigned columnNumber)
    : lineNumber(lineNumber), columnNumber(columnNumber) {
    if (lineNumber == 0) BUG("Line numbering should start at one");
}

cstring SourcePosition::toString() const {
    return absl::StrFormat("%d:%d", lineNumber, columnNumber);
}

//////////////////////////////////////////////////////////////////////////////////////////

SourceInfo::SourceInfo(const InputSources *sources, SourcePosition start, SourcePosition end)
    : sources(sources), start(start), end(end) {
    BUG_CHECK(sources != nullptr, "Invalid InputSources in SourceInfo");
    if (!start.isValid() || !end.isValid())
        BUG("Invalid source position in SourceInfo %1%-%2%", start.toString(), end.toString());
    if (start > end)
        BUG("SourceInfo position start %1% after end %2%", start.toString(), end.toString());
}

cstring SourceInfo::toString() const {
    return absl::StrFormat("(%s)-(%s)", start.toString().string_view(),
                           end.toString().string_view());
}

std::ostream &operator<<(std::ostream &os, const SourceInfo &info) {
    // FIXME: implement abseil stringify to skip cstring conversion here
    os << absl::StrFormat("(%s)-(%s)", info.start.toString().string_view(),
                          info.end.toString().string_view());
    return os;
}

//////////////////////////////////////////////////////////////////////////////////////////

InputSources::InputSources() : sealed(false) {
    mapLine("", 1);  // the first line read will be line 1 of stdin
    contents.push_back("");
}

void InputSources::addComment(SourceInfo srcInfo, bool singleLine, cstring body) {
    if (!singleLine)
        // Drop the "*/"
        body = body.exceptLast(2);
    comments.push_back(new Comment(srcInfo, singleLine, body));
}

/// prevent further changes
void InputSources::seal() {
    LOG4(toDebugString());
    if (sealed) BUG("InputSources already sealed");
    sealed = true;
}

unsigned InputSources::lineCount() const {
    int size = contents.size();
    if (contents.back().empty()) {
        // do not count the last line if it is empty.
        size -= 1;
        if (size < 0) BUG("Negative line count");
    }
    return size;
}

// Append this text to the last line
void InputSources::appendToLastLine(std::string_view text) {
    if (sealed) BUG("Appending to sealed InputSources");
    // Text should not contain any newline characters
    for (size_t i = 0; i < text.size(); i++) {
        char c = text[i];
        if (c == '\n') BUG("Text contains newlines");
    }
    contents.back() += text;
}

// Append a newline and start a new line
void InputSources::appendNewline(std::string_view newline) {
    if (sealed) BUG("Appending to sealed InputSources");
    contents.back() += newline;
    contents.push_back("");  // start a new line
}

void InputSources::appendText(const char *text) {
    if (text == nullptr) BUG("Null text being appended");
    std::string_view ref(text);

    while (ref.size() > 0) {
        auto nlPos = ref.find_first_of("\r\n");
        if (nlPos == std::string_view::npos) {
            appendToLastLine(ref);
            break;
        }

        size_t toCut = nlPos;
        if (toCut != 0) {
            std::string_view nonnl(ref.data(), toCut);
            appendToLastLine(nonnl);
            ref.remove_prefix(toCut);
        } else {
            if (ref[0] == '\n') {
                appendNewline("\n");
                ref.remove_prefix(1);
            } else if (ref.size() > 2 && ref[0] == '\r' && ref[1] == '\n') {
                appendNewline("\r\n");
                ref.remove_prefix(2);
            } else {
                // Just \r
                appendToLastLine(ref.substr(0, 1));
                ref.remove_prefix(1);
            }
        }
    }
}

std::string_view InputSources::getLine(unsigned lineNumber) const {
    if (lineNumber == 0) {
        return "";
        // BUG("Lines are numbered starting at 1");
        // don't throw: this code may be called by exceptions
        // reporting on elements that have no source position
    }
    return contents.at(lineNumber - 1);
}

void InputSources::mapLine(std::string_view file, unsigned originalSourceLineNo) {
    if (sealed) BUG("Changing mapping to sealed InputSources");
    unsigned lineno = getCurrentLineNumber();
    line_file_map.emplace(lineno, SourceFileLine(file, originalSourceLineNo));
}

SourceFileLine InputSources::getSourceLine(unsigned line) const {
    auto it = line_file_map.upper_bound(line);
    if (it == line_file_map.begin())
        // There must be always something mapped to line 0
        BUG("No source information for line %1%", line);
    LOG3(line << " mapped to " << it->first << "," << it->second.toString());
    --it;
    LOG3(line << " corrected to " << it->first << "," << it->second.toString());
    // For a source file such as
    // ----------
    // # 1 "x.p4"
    // parser start { }
    // ----------
    // The first line indicates that line 2 is the first line in x.p4
    // line=2, it->first=1, it->second.sourceLine=1
    // So we have to subtract one to get the real line number.
    const auto nominalLine = line - it->first + it->second.sourceLine;
    const auto realLine = nominalLine > 0 ? nominalLine - 1 : 0;
    return SourceFileLine(it->second.fileName.string_view(), realLine);
}

unsigned InputSources::getCurrentLineNumber() const { return contents.size(); }

SourcePosition InputSources::getCurrentPosition() const {
    unsigned line = getCurrentLineNumber();
    unsigned column = contents.back().size();
    return SourcePosition(line, column);
}

cstring InputSources::getSourceFragment(const SourcePosition &position, bool useMarker) const {
    SourceInfo info(this, position, position);
    return getSourceFragment(info, useMarker);
}

static std::string carets(std::string_view source, unsigned start, unsigned end) {
    std::stringstream builder;
    if (start > source.size()) start = source.size();

    unsigned i = 0;
    for (; i < start; i++) {
        char c = source[i];
        if (c == ' ' || c == '\t')
            builder.put(c);
        else
            builder.put(' ');
    }

    for (; i < std::max(end, start + 1); i++) builder.put('^');

    return builder.str();
}

cstring InputSources::getSourceFragment(const SourceInfo &position, bool useMarker) const {
    if (!position.isValid()) return ""_cs;

    // If the position spans multiple lines, truncate to just the first line
    if (position.getEnd().getLineNumber() > position.getStart().getLineNumber())
        return getSourceFragment(position.getStart(), useMarker);

    std::string_view result = getLine(position.getStart().getLineNumber());
    unsigned int start = position.getStart().getColumnNumber();
    unsigned int end = position.getEnd().getColumnNumber();

    // Normally result has a newline, but if not then we have to add a newline
    bool addNewline = !absl::StrContains(result, "\n");
    if (useMarker) {
        return absl::StrCat(result, addNewline ? "\n" : "", carets(result, start, end), "\n");
    }

    return absl::StrCat(result, addNewline ? "\n" : "", "\n");
}

cstring InputSources::getBriefSourceFragment(const SourceInfo &position) const {
    if (!position.isValid()) return ""_cs;

    std::string_view result = getLine(position.getStart().getLineNumber());
    unsigned int start = position.getStart().getColumnNumber();
    unsigned int end = position.getEnd().getColumnNumber();
    bool truncate = false;

    // If the position spans multiple lines, truncate to just the first line
    if (position.getEnd().getLineNumber() > position.getStart().getLineNumber()) {
        // go to the end of the first line
        end = result.size();
        if (absl::StrContains(result, "\n")) {
            --end;
        }
        truncate = true;
    }

    // Adding escape character in front of '"' character to properly store
    // quote marks as part of JSON properties, they must be escaped.
    if (absl::StrContains(result, "\"")) {
        auto out = absl::StrReplaceAll(result, {{"\"", "\\\""}});
        return out.substr(0, out.size() - 1);
    }

    return absl::StrCat(result.substr(start, end - start), truncate ? " ..." : "");
}

cstring InputSources::toDebugString() const {
    std::stringstream builder;
    for (const auto &line : contents) builder << line;
    builder << "---------------" << std::endl;
    for (const auto &lf : line_file_map)
        builder << lf.first << ": " << lf.second.toString() << std::endl;
    return builder.str();
}

///////////////////////////////////////////////////

cstring SourceInfo::toSourceFragment(bool useMarker) const {
    if (!isValid()) return ""_cs;
    return sources->getSourceFragment(*this, useMarker);
}

cstring SourceInfo::toBriefSourceFragment() const {
    if (!isValid()) return ""_cs;
    return sources->getBriefSourceFragment(*this);
}

cstring SourceInfo::toPositionString() const {
    if (!isValid()) return ""_cs;
    SourceFileLine position = sources->getSourceLine(start.getLineNumber());
    return position.toString();
}

cstring SourceInfo::toSourcePositionData(unsigned *outLineNumber, unsigned *outColumnNumber) const {
    SourceFileLine position = sources->getSourceLine(start.getLineNumber());
    if (outLineNumber != nullptr) {
        *outLineNumber = position.sourceLine;
    }
    if (outColumnNumber != nullptr) {
        *outColumnNumber = start.getColumnNumber();
    }
    return position.fileName;
}

SourceFileLine SourceInfo::toPosition() const {
    return sources->getSourceLine(start.getLineNumber());
}

cstring SourceInfo::getSourceFile() const {
    auto sourceLine = sources->getSourceLine(start.getLineNumber());
    return sourceLine.fileName;
}

cstring SourceInfo::getLineNum() const {
    SourceFileLine sourceLine = sources->getSourceLine(start.getLineNumber());
    return Util::toString(sourceLine.sourceLine);
}

////////////////////////////////////////////////////////

cstring SourceFileLine::toString() const {
    return absl::StrFormat("%s(%d)", fileName.string_view(), sourceLine);
}

}  // namespace Util

////////////////////////////////////////////////////////

[[gnu::used]]  // ensure linker will not drop function even if unused
void dbprint(const IHasDbPrint *o) {
    o->dbprint(std::cout);
    std::cout << std::endl << std::flush;
}
