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

void IHasDbPrint::print() const { dbprint(std::cout); std::cout << std::endl; }

namespace Util {
SourcePosition::SourcePosition(unsigned lineNumber, unsigned columnNumber)
        : lineNumber(lineNumber),
          columnNumber(columnNumber) {
    if (lineNumber == 0)
        BUG("Line numbering should start at one");
}

cstring SourcePosition::toString() const {
    return Util::printf_format("%d:%d", this->lineNumber, this->columnNumber);
}


//////////////////////////////////////////////////////////////////////////////////////////

SourceInfo::SourceInfo(SourcePosition start, SourcePosition end) :
        start(start),
        end(end) {
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

InputSources* InputSources::instance = new InputSources();

InputSources::InputSources() :
        sealed(false) {
    this->mapLine(nullptr, 0);
    this->contents.push_back("");
}

/* static */ void InputSources::reset() {
    instance = new InputSources;
}

// prevent further changes
void InputSources::seal() {
    if (this->sealed)
        BUG("InputSources already sealed");
    this->sealed = true;
}

unsigned InputSources::lineCount() const {
    int size = this->contents.size();
    if (this->contents.back().isNullOrEmpty()) {
        // do not count the last line if it is empty.
        size -= 1;
        if (size < 0)
            BUG("Negative line count");
    }
    return size;
}

// Append this text to the last line
void InputSources::appendToLastLine(StringRef text) {
    if (this->sealed)
        BUG("Appending to sealed InputSources");
    // Text should not contain any newline characters
    for (size_t i = 0; i < text.len; i++) {
        char c = text[i];
        if (c == '\n')
            BUG("Text contains newlines");
    }
    this->contents.back() += text.toString();
}

// Append a newline and start a new line
void InputSources::appendNewline(StringRef newline) {
    if (this->sealed)
        BUG("Appending to sealed InputSources");
    this->contents.back() += newline.toString();
    this->contents.push_back("");  // start a new line
}

void InputSources::appendText(const char* text) {
    if (text == nullptr)
        BUG("Null text being appended");
    StringRef ref = text;

    while (ref.len > 0) {
        const char* nl = ref.find("\r\n");
        if (nl == nullptr) {
            this->appendToLastLine(ref);
            break;
        }

        size_t toCut = nl - ref.p;
        if (toCut != 0) {
            StringRef nonnl(ref.p, toCut);
            this->appendToLastLine(nonnl);
            ref += toCut;
        } else {
            if (ref[0] == '\n') {
                this->appendNewline("\n");
                ref += 1;
            } else if (ref.len > 2 && ref[0] == '\r' && ref[1] == '\n') {
                this->appendNewline("\r\n");
                ref += 2;
            } else {
                // Just \r
                this->appendToLastLine(ref.substr(0, 1));
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
    return this->contents.at(lineNumber - 1);
}

void InputSources::mapLine(cstring file, unsigned originalSourceLineNo) {
    if (this->sealed)
        BUG("Changing mapping to sealed InputSources");
    unsigned lineno = this->getCurrentLineNumber();
    this->line_file_map.emplace(lineno, SourceFileLine(file, originalSourceLineNo));
}

SourceFileLine InputSources::getSourceLine(unsigned line) const {
    auto it = this->line_file_map.upper_bound(line);
    if (it == this->line_file_map.begin())
        // There must be always something mapped to line 0
        BUG("No source information for line %1%", line);
    --it;
    // For a source file such as
    // ----------
    // # 1 "x.p4"
    // parser start { }
    // ----------
    // The first line indicates that line 2 is the first line in x.p4
    // line=2, it->first=1, it->second.sourceLine=1
    // So we have to subtract one to get the real line number
    return SourceFileLine(it->second.fileName, line - it->first + it->second.sourceLine - 1);
}

unsigned InputSources::getCurrentLineNumber() const {
    return this->contents.size();
}

SourcePosition InputSources::getCurrentPosition() const {
    unsigned line = this->getCurrentLineNumber();
    unsigned column = this->contents.back().size();
    return SourcePosition(line, column);
}

cstring InputSources::getSourceFragment(const SourcePosition &position) const {
    SourceInfo info(position, position);
    return this->getSourceFragment(info);
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
        return this->getSourceFragment(position.getStart());

    cstring result = this->getLine(position.getStart().getLineNumber());
    // Normally result has a newline, but if not
    // then we have to add a newline
    cstring toadd = "";
    if (result.find('\n') == nullptr)
        toadd = cstring::newline;
    cstring marker = carets(result, position.getStart().getColumnNumber(),
                            position.getEnd().getColumnNumber());
    return result + toadd + marker + cstring::newline;
}

cstring InputSources::toDebugString() const {
    std::stringstream builder;
    for (auto line : this->contents)
        builder << line;
    builder << "---------------" << std::endl;
    for (auto lf : this->line_file_map)
        builder << lf.first << ": " << lf.second.toString() << std::endl;
    return cstring(builder.str());
}

///////////////////////////////////////////////////

cstring SourceInfo::toSourceFragment() const {
    return InputSources::instance->getSourceFragment(*this);
}

cstring SourceInfo::toPositionString() const {
    if (!this->isValid())
        return "";
    SourceFileLine position = InputSources::instance->getSourceLine(this->start.getLineNumber());
    return position.toString();
}

SourceFileLine SourceInfo::toPosition() const {
    return InputSources::instance->getSourceLine(this->start.getLineNumber());
}

////////////////////////////////////////////////////////

cstring SourceFileLine::toString() const {
    return Util::printf_format("%s(%d)", this->fileName.c_str(), this->sourceLine);
}

}  // namespace Util
