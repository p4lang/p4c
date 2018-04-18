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

#include <sstream>

#include <algorithm>
#include "source_file.h"
#include "exceptions.h"
#include "lib/log.h"

void IHasDbPrint::print() const { dbprint(std::cout); std::cout << std::endl; }

namespace Util {
SourcePosition::SourcePosition(unsigned lineNumber, unsigned columnNumber)
        : lineNumber(lineNumber),
          columnNumber(columnNumber) {
    if (lineNumber == 0)
        BUG("Line numbering should start at one");
}

cstring SourcePosition::toString() const {
    return Util::printf_format("%d:%d", lineNumber, columnNumber);
}


//////////////////////////////////////////////////////////////////////////////////////////

SourceInfo::SourceInfo(const InputSources* sources, SourcePosition start,
                       SourcePosition end)
        : sources(sources), start(start), end(end) {
    BUG_CHECK(sources != nullptr, "Invalid InputSources in SourceInfo");
    if (!start.isValid() || !end.isValid())
        BUG("Invalid source position in SourceInfo %1%-%2%",
                          start.toString(), end.toString());
    if (start > end)
        BUG("SourceInfo position start %1% after end %2%",
                          start.toString(), end.toString());
}

cstring SourceInfo::toDebugString() const {
    return Util::printf_format("(%s)-(%s)", start.toString(), end.toString());
}

//////////////////////////////////////////////////////////////////////////////////////////

InputSources::InputSources() : sealed(false) {
    mapLine(nullptr, 1);  // the first line read will be line 1 of stdin
    contents.push_back("");
}

void InputSources::addComment(SourceInfo srcInfo, bool singleLine, cstring body) {
    if (!singleLine)
        // Drop the "*/"
        body = body.exceptLast(2);
    auto comment = new Comment(srcInfo, singleLine, body);
    comments.push_back(comment);
}

/// prevent further changes
void InputSources::seal() {
    LOG4(toDebugString());
    if (sealed)
        BUG("InputSources already sealed");
    sealed = true;
}

unsigned InputSources::lineCount() const {
    int size = contents.size();
    if (contents.back().isNullOrEmpty()) {
        // do not count the last line if it is empty.
        size -= 1;
        if (size < 0)
            BUG("Negative line count");
    }
    return size;
}

// Append this text to the last line
void InputSources::appendToLastLine(StringRef text) {
    if (sealed)
        BUG("Appending to sealed InputSources");
    // Text should not contain any newline characters
    for (size_t i = 0; i < text.len; i++) {
        char c = text[i];
        if (c == '\n')
            BUG("Text contains newlines");
    }
    contents.back() += text.toString();
}

// Append a newline and start a new line
void InputSources::appendNewline(StringRef newline) {
    if (sealed)
        BUG("Appending to sealed InputSources");
    contents.back() += newline.toString();
    contents.push_back("");  // start a new line
}

void InputSources::appendText(const char* text) {
    if (text == nullptr)
        BUG("Null text being appended");
    StringRef ref = text;

    while (ref.len > 0) {
        const char* nl = ref.find("\r\n");
        if (nl == nullptr) {
            appendToLastLine(ref);
            break;
        }

        size_t toCut = nl - ref.p;
        if (toCut != 0) {
            StringRef nonnl(ref.p, toCut);
            appendToLastLine(nonnl);
            ref += toCut;
        } else {
            if (ref[0] == '\n') {
                appendNewline("\n");
                ref += 1;
            } else if (ref.len > 2 && ref[0] == '\r' && ref[1] == '\n') {
                appendNewline("\r\n");
                ref += 2;
            } else {
                // Just \r
                appendToLastLine(ref.substr(0, 1));
                ref += 1;
            }
        }
    }
}

cstring InputSources::getLine(unsigned lineNumber) const {
    if (lineNumber == 0) {
        return "";
        // BUG("Lines are numbered starting at 1");
        // don't throw: this code may be called by exceptions
        // reporting on elements that have no source position
    }
    return contents.at(lineNumber - 1);
}

void InputSources::mapLine(cstring file, unsigned originalSourceLineNo) {
    if (sealed)
        BUG("Changing mapping to sealed InputSources");
    unsigned lineno = getCurrentLineNumber();
    line_file_map.emplace(lineno, SourceFileLine(file, originalSourceLineNo));
}

SourceFileLine InputSources::getSourceLine(unsigned line) const {
    auto it = line_file_map.upper_bound(line+1);
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
    return SourceFileLine(it->second.fileName, realLine);
}

unsigned InputSources::getCurrentLineNumber() const {
    return contents.size();
}

SourcePosition InputSources::getCurrentPosition() const {
    unsigned line = getCurrentLineNumber();
    unsigned column = contents.back().size();
    return SourcePosition(line, column);
}

cstring InputSources::getSourceFragment(const SourcePosition &position) const {
    SourceInfo info(this, position, position);
    return getSourceFragment(info);
}

cstring carets(cstring source, unsigned start, unsigned end) {
    std::stringstream builder;
    if (start > source.size())
        start = source.size();

    unsigned i;
    for (i=0; i < start; i++) {
        char c = source.c_str()[i];
        if (c == ' ' || c == '\t')
            builder.put(c);
        else
            builder.put(' ');
    }

    for (; i < std::max(end, start+1); i++)
        builder.put('^');

    return builder.str();
}

cstring InputSources::getSourceFragment(const SourceInfo &position) const {
    if (!position.isValid())
        return "";

    // If the position spans multiple lines, truncate to just the first line
    if (position.getEnd().getLineNumber() > position.getStart().getLineNumber())
        return getSourceFragment(position.getStart());

    cstring result = getLine(position.getStart().getLineNumber());
    // Normally result has a newline, but if not
    // then we have to add a newline
    cstring toadd = "";
    if (result.find('\n') == nullptr)
        toadd = cstring::newline;
    cstring marker = carets(result, position.getStart().getColumnNumber(),
                            position.getEnd().getColumnNumber());
    return result + toadd + marker + cstring::newline;
}

cstring InputSources::getBriefSourceFragment(const SourceInfo &position) const {
    if (!position.isValid())
        return "";

    cstring result = getLine(position.getStart().getLineNumber());
    unsigned int start = position.getStart().getColumnNumber();
    unsigned int end;
    cstring toadd = "";

    // If the position spans multiple lines, truncate to just the first line
    if (position.getEnd().getLineNumber() > position.getStart().getLineNumber()) {
        // go to the end of the first line
        end = result.size();
        if (result.find('\n') != nullptr) {
            --end;
        }
        toadd = " ...";
    } else {
        end = position.getEnd().getColumnNumber();
    }
    return result.substr(start, end - start) + toadd;
}

cstring InputSources::toDebugString() const {
    std::stringstream builder;
    for (auto line : contents)
        builder << line;
    builder << "---------------" << std::endl;
    for (auto lf : line_file_map)
        builder << lf.first << ": " << lf.second.toString() << std::endl;
    return cstring(builder.str());
}

///////////////////////////////////////////////////

cstring SourceInfo::toSourceFragment() const {
    return sources->getSourceFragment(*this);
}

cstring SourceInfo::toBriefSourceFragment() const {
    return sources->getBriefSourceFragment(*this);
}

cstring SourceInfo::toPositionString() const {
    if (!isValid())
        return "";
    SourceFileLine position = sources->getSourceLine(start.getLineNumber());
    return position.toString();
}

cstring SourceInfo::toSourcePositionData(unsigned *outLineNumber,
                                         unsigned *outColumnNumber) const {
    SourceFileLine position = sources->getSourceLine(start.getLineNumber());
    if (outLineNumber != nullptr) {
        *outLineNumber = position.sourceLine;
    }
    if (outColumnNumber != nullptr) {
        *outColumnNumber = start.getColumnNumber();
    }
    return position.fileName.c_str();
}

SourceFileLine SourceInfo::toPosition() const {
    return sources->getSourceLine(start.getLineNumber());
}

cstring SourceInfo::getSourceFile() const {
    auto sourceLine = sources->getSourceLine(start.getLineNumber());
    return sourceLine.fileName;
}

////////////////////////////////////////////////////////

cstring SourceFileLine::toString() const {
    return Util::printf_format("%s(%d)", fileName.c_str(), sourceLine);
}

}  // namespace Util
